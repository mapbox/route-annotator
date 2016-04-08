/************************************************************
 * Route Annotator
 *
 * This HTTP service takes a list of OSM nodeids or coordinates,
 * and returns all the tags on the ways that those nodeids
 * touch.
 *
 * When using coordinates, they must be within 1m of the coordinates
 * loaded from the OSM file.
 ************************************************************/
#include <cstddef>
#include <cstdlib>
#include <cstdio>

#include <string>
#include <vector>
#include <future>
#include <tuple>
#include <utility>
#include <iterator>
#include <stdexcept>
#include <system_error>
#include <iostream>
#include <algorithm>
#include <unordered_set>

#ifndef NDEBUG
#define BOOST_SPIRIT_X3_DEBUG
#endif

#include <boost/spirit/home/x3.hpp>

#include <cpprest/http_listener.h>
#include <cpprest/http_msg.h>
#include <cpprest/json.h>

#include <errno.h>
#include <signal.h>

// Basic libosmium includes
#include <osmium/osm/types.hpp>
#include <osmium/handler.hpp>
#include <osmium/io/file.hpp>
#include <osmium/visitor.hpp>
// We take any input that libosmium supports (XML, PBF, osm.bz2, etc)
#include <osmium/io/any_input.hpp>

// Needed for lon/lat lookups inside way handler
#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/index/map/all.hpp>

// For iterating over pairs of nodes/coordinates
#include <boost/iterator/zip_iterator.hpp>
#include <boost/tuple/tuple.hpp>

// Used to constrct the RTree for finding nodes near lon/lat
#include <boost/geometry.hpp>
#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/index/rtree.hpp>

#include <boost/timer/timer.hpp>

// Type declarations for node ids and way ids.  We don't need the full 64 bits
// for ways as the highest way number isn't close to 2^32 yet.
typedef std::uint64_t external_nodeid_t;
typedef std::uint32_t internal_nodeid_t;
typedef std::uint32_t tagsetid_t;

// Node indexing types for libosmium
// We need these because we need access to the lon/lat for the noderefs inside the way
// callback in our handler.
typedef osmium::index::map::Dummy<osmium::unsigned_object_id_type, osmium::Location> index_neg_type;
typedef osmium::index::map::SparseMemArray<osmium::unsigned_object_id_type, osmium::Location> index_pos_type;
//typedef osmium::index::map::DenseMmapArray<osmium::unsigned_object_id_type, osmium::Location> index_pos_type;
typedef osmium::handler::NodeLocationsForWays<index_pos_type, index_neg_type> location_handler_type;

// Data types for the RTree
//typedef boost::geometry::model::point<double, 2, boost::geometry::cs::geographic<boost::geometry::degree>> point_t;
typedef boost::geometry::model::point<double, 2, boost::geometry::cs::spherical_equatorial<boost::geometry::degree>> point_t;
//typedef boost::geometry::model::point<double, 2, boost::geometry::cs::cartesian> point_t;
typedef std::pair<point_t, internal_nodeid_t> value_t;

// Data types for our lookup tables
typedef std::pair<internal_nodeid_t, internal_nodeid_t> internal_nodepair_t;

typedef std::unordered_map<external_nodeid_t, internal_nodeid_t> external_internal_map_t;

// Our tagmap is a set of indexes.  We don't store the strings themselves, we store
// indexes into our string lookup tables, which only stores each unique string once.
typedef std::vector<std::pair<std::uint32_t, std::uint32_t>> uint32_pairvector_t;

// Custom hashing functions for our data types to use in standard containers (sets, maps, etc).
namespace std {
// This is so that we have reasonable hashing of pairs of nodeids
template <> struct hash<internal_nodepair_t> {
    inline size_t operator()(const internal_nodepair_t &v) const {
        return (static_cast<std::uint64_t>(v.first) << 32) + v.second;
    }
};

}

struct Datacontainer final {
    /**
     * The public data structures, this is where everything gets indexed.
     */
    std::unordered_map<internal_nodepair_t, tagsetid_t> pair_tagset_map;
    uint32_pairvector_t tagset_list;
    uint32_pairvector_t key_value_pairs;
    std::unique_ptr<boost::geometry::index::rtree<value_t, boost::geometry::index::rstar<8>>> rtree;

    std::unordered_map<external_nodeid_t, internal_nodeid_t> external_internal_map;

        /**
     * This needs to be called once all ways are processed.  It constructs the RTree object using the accumulated data,
     * and frees the memory for the no-longer-needed string index and used nodes list.
     */
    void finish()
    {
        rtree = std::make_unique<boost::geometry::index::rtree<value_t, boost::geometry::index::rstar<8>>>(used_nodes_list.begin(), used_nodes_list.end());

        // Tricks to free memory, swap out data with empty versions
        // This frees the memory.  shrink_to_fit doesn't guarantee that.
        std::vector<value_t>().swap(used_nodes_list);
        std::unordered_map<std::string, std::uint32_t>().swap(string_index);

        // Hint that these data structures can be shrunk.
        string_data.shrink_to_fit();
        string_offsets.shrink_to_fit();

        tagset_list.shrink_to_fit();
        key_value_pairs.shrink_to_fit();
        string_offsets.shrink_to_fit();
    }

    /**
     * Given a stringid, finds it in the offset/length table, then returns a new
     * string object by coping offset+length bytes from the big string buffer.
     **/
    std::string getstring(std::size_t stringid)
    {
        auto stringinfo = string_offsets[stringid];
        std::string result(string_data.begin() + stringinfo.first, string_data.begin() + stringinfo.first + stringinfo.second);
        return result;
    }

    std::unordered_map<std::string, std::uint32_t> string_index;
    std::vector<value_t> used_nodes_list;

    std::vector<char> string_data;
    uint32_pairvector_t string_offsets;

    // Adds a string to the data blob, returns its index
    std::uint32_t addstring(const char * str)
    {
        auto idx = string_index.find(str);
        if (idx == string_index.end())
        {
            BOOST_ASSERT(string_index.size() < std::numeric_limits<std::uint32_t>::max());
            string_index.emplace(str, static_cast<uint32_t>(string_index.size()));
            auto string_length = static_cast<std::uint32_t>(std::min<std::size_t>(255, std::strlen(str)));
            std::copy(str, str + string_length, std::back_inserter(string_data));
            BOOST_ASSERT(string_data.size() < std::numeric_limits<std::uint32_t>::max());
            BOOST_ASSERT(string_data.size() >= string_length);
            string_offsets.emplace_back(static_cast<std::uint32_t>(string_data.size()) - string_length, string_length);
            idx = string_index.find(str);
        }
        BOOST_ASSERT(idx->second < std::numeric_limits<std::uint32_t>::max());
        return static_cast<std::uint32_t>(idx->second);
    }

    void dump()
    {
        std::cout << "String data is " << (string_data.capacity() * sizeof(char)) << " Used: " << (string_data.size() * sizeof(char)) << "\n";
        std::cout << "tagset = Allocated " << (tagset_list.capacity() * sizeof(uint32_pairvector_t::value_type)) << "  Used: " << (tagset_list.size() * sizeof(uint32_pairvector_t::value_type)) << "\n";
        std::cout << "keyvalue = Allocated " << (key_value_pairs.capacity() * sizeof(uint32_pairvector_t::value_type)) << "  Used: " << (key_value_pairs.size() * sizeof(uint32_pairvector_t::value_type)) << "\n";
        std::cout << "stringoffset = Allocated " << (string_offsets.capacity() * sizeof(uint32_pairvector_t::value_type)) << "  Used: " << (string_offsets.size() * sizeof(uint32_pairvector_t::value_type)) << "\n";


        std::cout << "pair_tagset_map = Allocated " << (pair_tagset_map.size() * (sizeof(uint32_pairvector_t::value_type) + sizeof(tagsetid_t))) << "  Load factor: " << pair_tagset_map.load_factor() << " Buckets: " << pair_tagset_map.bucket_count() << "\n";

    }



};

/**
 * The handler for libosmium.  This class basically contains one callback that's called by
 * libosmium as it parses our OSM file.  The `way` function adds data to the class member
 * variables as ways are discovered.
 * This object can then be used to lookup nodes and way tags.
 */
struct Extractor final : osmium::handler::Handler {

    Datacontainer * c;

    Extractor(Datacontainer *container)
    {
        c = container;
    }

    /**
     * Handler for ways as they are seen
     */
    void way(const osmium::Way& way) {

        // TODO: check if way is interesting (road, highway, etc), there's no need to keep
        // everything....
        const char *oneway = way.tags().get_value_by_key("one_way");
        bool forward = (!oneway || std::strcmp(oneway, "yes") == 0);
        bool reverse = (!oneway || std::strcmp(oneway, "-1") == 0);

        const char *highway = way.tags().get_value_by_key("highway");
        bool usable = highway && (
                std::strcmp(highway, "motorway") == 0 ||
                std::strcmp(highway, "motorway_link") == 0 ||
                std::strcmp(highway, "trunk") == 0 ||
                std::strcmp(highway, "trunk_link") == 0 ||
                std::strcmp(highway, "primary") == 0 ||
                std::strcmp(highway, "primary_link") == 0 ||
                std::strcmp(highway, "secondary") == 0 ||
                std::strcmp(highway, "secondary_link") == 0 ||
                std::strcmp(highway, "tertiary") == 0 ||
                std::strcmp(highway, "tertiary_link") == 0 ||
                std::strcmp(highway, "residential") == 0 ||
                std::strcmp(highway, "living_street") == 0 ||
                std::strcmp(highway, "unclassified") == 0 ||
                std::strcmp(highway, "service") == 0 ||
                std::strcmp(highway, "ferry") == 0 ||
                std::strcmp(highway, "movable") == 0 ||
                std::strcmp(highway, "shuttle_train") == 0 ||
                std::strcmp(highway, "default") == 0);

        if (usable && way.nodes().size() > 1 && (forward || reverse))
        {

            BOOST_ASSERT(c->key_value_pairs.size() < std::numeric_limits<std::uint32_t>::max());
            const auto tagstart = static_cast<std::uint32_t>(c->key_value_pairs.size());
            // Create a map of the tags for this way, add the strings to the stringbuffer
            // and then add the tag map to the way map.
            for (const osmium::Tag& tag : way.tags()) {
                const auto key_pos = c->addstring(tag.key());
                const auto val_pos = c->addstring(tag.value());
                c->key_value_pairs.emplace_back(key_pos, val_pos);
            }
            c->key_value_pairs.emplace_back(c->addstring("_way_id"),c->addstring(std::to_string(way.id()).c_str()));
            BOOST_ASSERT(c->key_value_pairs.size() < std::numeric_limits<std::uint32_t>::max());
            const auto tagend = static_cast<std::uint32_t>(c->key_value_pairs.size()-1);
            c->tagset_list.emplace_back(tagstart, tagend);

            BOOST_ASSERT(c->tagset_list.size() < std::numeric_limits<tagsetid_t>::max());
            const auto tagset_id = static_cast<tagsetid_t>(c->tagset_list.size()-1);


            // This iterates over each pair of nodes.
            // Given the nodes 1,2,3,4,5,6
            // This loop will be called with (1,2), (2,3), (3,4), (4,5), (5, 6)
            std::for_each(
                boost::make_zip_iterator(boost::make_tuple(way.nodes().cbegin(), way.nodes().cbegin()+1)),
                boost::make_zip_iterator(boost::make_tuple(way.nodes().cend()-1, way.nodes().cend())),
                [this,tagset_id,forward,reverse](boost::tuple<const osmium::NodeRef, const osmium::NodeRef> pair) {
                    try {
                        point_t a{pair.get<0>().location().lon(), pair.get<0>().location().lat()};
                        point_t b{pair.get<1>().location().lon(), pair.get<1>().location().lat()};

                        internal_nodeid_t internal_a_id = 0;
                        internal_nodeid_t internal_b_id = 0;

                        const auto tmp_a = c->external_internal_map.find(pair.get<0>().ref());

                        if (tmp_a == c->external_internal_map.end())
                        {
                            internal_a_id = static_cast<internal_nodeid_t>(c->used_nodes_list.size());
                            c->used_nodes_list.emplace_back(a, internal_a_id);
                            c->external_internal_map.emplace(pair.get<0>().ref(), internal_a_id);
                        }
                        else
                        {
                            internal_a_id = tmp_a->second;
                        }

                        const auto tmp_b = c->external_internal_map.find(pair.get<1>().ref());
                        if (tmp_b == c->external_internal_map.end())
                        {
                            internal_b_id = static_cast<internal_nodeid_t>(c->used_nodes_list.size());
                            c->used_nodes_list.emplace_back(b, internal_b_id);
                            c->external_internal_map.emplace(pair.get<1>().ref(), internal_b_id);
                        }
                        else
                        {
                            internal_b_id = tmp_b->second;
                        }


                        if (forward) {
                            c->pair_tagset_map.emplace(std::make_pair(internal_a_id, internal_b_id), tagset_id);
                        }
                        if (reverse) {
                            c->pair_tagset_map.emplace(std::make_pair(internal_b_id, internal_a_id), tagset_id);
                        }
                    }
                    catch (const osmium::invalid_location &e)
                    {
                        //std::cerr << "WARNING: Invalid location for one of nodes " << pair.get<0>().ref() << " or " << pair.get<1>().ref() << "\n";
                    }
                }
            );
        }
    }

};

// Fulfilled promise signals clean shutdown
static std::promise<void> signal_shutdown;

int main(int argc, char** argv) try {
  if (argc != 3) {
    std::fprintf(stderr, "Usage: %s uri <filename.[pbf|osm|osm.bz2|osm.gz]>\n", argv[0]);
    return EXIT_FAILURE;
  }

  // Register CTRL+C handler on POSIX systems for clean shutdown
  struct ::sigaction action;
  sigemptyset(&action.sa_mask);
  action.sa_flags = SA_SIGINFO;
  action.sa_sigaction = [](int, ::siginfo_t*, void*) { signal_shutdown.set_value(); };

  if (::sigaction(SIGINT, &action, nullptr) == -1)
    throw std::system_error{errno, std::system_category()};

  // This is our callback handler, it ends up with all the data
  Datacontainer data;

  // Load OSM file using libosmium
  // Do this in a block so that libosmium gets cleaned up with RAII
  {
    Extractor extractor(&data);
    std::cerr << "Parsing " << argv[2] << " ... " << std::flush;
    osmium::io::File osmfile{argv[2]};
    osmium::io::Reader fileReader(osmfile, osmium::osm_entity_bits::way | osmium::osm_entity_bits::node);
    int fd = open("nodes.cache", O_RDWR | O_CREAT, 0666);
    if (fd == -1) {
        std::cerr << "Can not open node cache file '" << argv[2] << "': " << strerror(errno) << "\n";
        return 1;
    }
    index_pos_type index_pos {fd};
    index_neg_type index_neg;
    location_handler_type location_handler(index_pos, index_neg);
    location_handler.ignore_errors();
    osmium::apply(fileReader, location_handler, extractor);
    std::cerr << "done\n";
    std::cerr << "Number of node pairs indexed: " << data.pair_tagset_map.size() << "\n";
    std::cerr << "Number of ways indexed: " << data.tagset_list.size() << "\n";
  }

  std::cerr << "Constructing RTree ... " << std::flush;
  data.finish();
  std::cerr << "done\n" << std::flush;
  data.dump();


  // Setup the web server
  // Server listens based on URI (Host, Port)
  web::uri uri{argv[1]};
  web::http::experimental::listener::http_listener listener{uri};

  // GET Request Handler
  listener.support(web::http::methods::GET, [&data](const auto& request) {
    const auto uri = request.relative_uri();
    const auto path = uri.path();
    const auto query = uri.query();

    // Log the request
    std::fprintf(stderr, "%s\t%s\t%s\n", request.method().c_str(), path.c_str(), query.c_str());

    if (path == "/")
    {
        request.reply(web::http::status_codes::OK);
        return;
    }

    // Parse Coordinates with X3
    // First, try the /nodelist URL pattern
    const auto nodelist_parser = "/nodelist/" >> (boost::spirit::x3::ulong_long) % ",";
    std::vector<external_nodeid_t> external_nodeids;
    std::vector<internal_nodeid_t> internal_nodeids;
    std::vector<double> coordinates;

    // If /nodelist fails, we'll try /coordlist
    if (!parse(begin(path), end(path), nodelist_parser, external_nodeids) || external_nodeids.size() < 2) {

        const auto coordlist_parser = "/coordlist/" >> (boost::spirit::x3::double_ >> "," >> boost::spirit::x3::double_ ) % ";";

        // Both url forms failed, so we'll just return an error message
        if (!parse(begin(path), end(path), coordlist_parser, coordinates) || coordinates.size() < 2) {
            web::json::value response;
            response["message"] = web::json::value("Bad request");
            request.reply(web::http::status_codes::BadRequest,response);
            return; // Early Exit
        }

        // Loop over the coordinates, and try to find nearby matches in the rtree
        // Limit to 2m radius.  If there are multiple matches, take the closest one
        //boost::geometry::strategy::distance::haversine<double> const haversine(6372795.0);
        boost::geometry::strategy::distance::haversine<double> const haversine(6372795.0);

        for (std::size_t lonIdx{0}, latIdx{1} ; latIdx < coordinates.size(); lonIdx+=2, latIdx+=2)
        {
            point_t pt{coordinates[lonIdx],coordinates[latIdx]};

            std::vector<value_t> results;

            // Search for the nearest point to the one supplied
            data.rtree->query(boost::geometry::index::nearest(pt, 1), std::back_inserter(results));

            // If we got exactly one hit (we should), append it to our nodeids list, if it was within 1m
            if (results.size() == 1 && boost::geometry::distance(pt, results[0].first) < 1)
            {
                internal_nodeids.push_back(results[0].second);
            } 
            // otherwise, insert a 0 value, this coordinate didn't match
            else 
            {
                internal_nodeids.push_back(std::numeric_limits<internal_nodeid_t>::max());
            }
        }
    } else {
        // Convert external node ids into internal ones
        std::for_each(external_nodeids.begin(), external_nodeids.end(), [&data,&external_nodeids](const external_nodeid_t n) {
                const auto internal_node_id = data.external_internal_map.find(n);
                if (internal_node_id == data.external_internal_map.end())
                {
                    // Push an invalid nodeid into the list if we didn't match
                    external_nodeids.push_back(std::numeric_limits<internal_nodeid_t>::max());
                }
                else
                {
                    external_nodeids.push_back(internal_node_id->second);
                }
                });
    }

    // If after all of the above, we still don't have at least two nodes, this is a bad request
    if (internal_nodeids.size() < 2)
    {
      web::json::value response;
      response["message"] = web::json::value("Bad request");
      request.reply(web::http::status_codes::BadRequest,response);
    }

    web::json::value response;


    std::vector<int> tagset_offset_list;
    std::unordered_map<tagsetid_t, int> tagset_indexes;
    std::vector<tagsetid_t> tagset_list;

    // the zip_iterator allows us to group pairs.  So, if we get 1,2,3,4
    // in our nodeids vector, this for_each will loop over:
    //   (1,2) (2,3) (3,4)
    std::for_each(
        boost::make_zip_iterator(boost::make_tuple(internal_nodeids.cbegin(), internal_nodeids.cbegin()+1)),
        boost::make_zip_iterator(boost::make_tuple(internal_nodeids.cend()-1, internal_nodeids.cend())),
        [&data,&tagset_offset_list,&tagset_indexes,&tagset_list](boost::tuple<const internal_nodeid_t, const internal_nodeid_t> pair) {
            // If our pair exists in the pair_tagset_map
            const auto tagset_id = data.pair_tagset_map.find(std::make_pair(pair.get<0>(), pair.get<1>()));
            if (tagset_id != data.pair_tagset_map.end())
            {
                if (tagset_indexes.find(tagset_id->second) == tagset_indexes.end())
                {
                    tagset_list.push_back(tagset_id->second);
                    tagset_indexes[tagset_id->second] = static_cast<int>(tagset_list.size()-1);
                }
                tagset_offset_list.push_back(tagset_indexes.find(tagset_id->second)->second);
            }
            else
            {
                tagset_offset_list.push_back(-1);
            }
        }
    );

    web::json::value j_array = web::json::value::array(tagset_offset_list.size());
    int idx = 0;
    for (const auto &offset : tagset_offset_list) {
        j_array[idx++] = offset;
    }

    response["tagset_indexes"] = j_array;

    web::json::value waydata = web::json::value::array(tagset_list.size());

    idx = 0;
    for (const auto &tagset_id : tagset_list)
    {
        // Then get the tags for that way
        const auto tagset = data.tagset_list[tagset_id];

        web::json::value tags = web::json::value::object();
        for (auto i = tagset.first; i<= tagset.second; i++)
        {
            const auto key_value_pair = data.key_value_pairs[i];
            tags[data.getstring(key_value_pair.first)] = web::json::value(data.getstring(key_value_pair.second));
        }
        waydata[idx++] = tags;
    }

    response["way_tags"] = waydata;

    // Send the reply back
    request.reply(web::http::status_codes::OK, response);
  });


  // Start up server, handles concurrency internally
  listener.open().then([&uri] { std::fprintf(stderr, "Host %s\nPort %d\n\n", uri.host().c_str(), uri.port()); }).wait();

  // Main thread blocks on future until its associated promise is fulfilled from within the CTRL+C handler
  signal_shutdown.get_future().wait();

  // Only then we shutdown the server
  listener.close().then([] { std::fprintf(stderr, "\nBye!\n"); }).wait();

} catch (const std::exception& e) {
  std::fprintf(stderr, "Error: %s\n", e.what());
  return EXIT_FAILURE;
}
