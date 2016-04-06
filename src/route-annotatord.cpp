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

#ifdef __unix__
#include <errno.h>
#include <signal.h>
#endif

// Basic libosmium includes
#include <osmium/osm/types.hpp>
#include <osmium/handler.hpp>
#include <osmium/io/file.hpp>
#include <osmium/visitor.hpp>
// We take any input that libosmium supports (XML, PBF, osm.bz2, etc)
#include <osmium/io/any_input.hpp>

// Needed for lon/lat lookups inside way handler
#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/index/map/dummy.hpp>
#include <osmium/index/map/sparse_mem_array.hpp>

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
typedef std::uint64_t nodeid_t;
typedef std::uint32_t wayid_t;

// Node indexing types for libosmium
// We need these because we need access to the lon/lat for the noderefs inside the way
// callback in our handler.
typedef osmium::index::map::Dummy<osmium::unsigned_object_id_type, osmium::Location> index_neg_type;
typedef osmium::index::map::SparseMemArray<osmium::unsigned_object_id_type, osmium::Location> index_pos_type;
typedef osmium::handler::NodeLocationsForWays<index_pos_type, index_neg_type> location_handler_type;

// Data types for the RTree
//typedef boost::geometry::model::point<double, 2, boost::geometry::cs::geographic<boost::geometry::degree>> point_t;
typedef boost::geometry::model::point<double, 2, boost::geometry::cs::spherical_equatorial<boost::geometry::degree>> point_t;
//typedef boost::geometry::model::point<double, 2, boost::geometry::cs::cartesian> point_t;
typedef std::pair<point_t, nodeid_t> value_t;

// Data types for our lookup tables
typedef std::pair<nodeid_t, nodeid_t> nodepair_t;

// Our tagmap is a set of indexes.  We don't store the strings themselves, we store
// indexes into our string lookup tables, which only stores each unique string once.
typedef std::unordered_map<std::size_t,std::size_t> tagmap_t;

// Custom hashing functions for our data types to use in standard containers (sets, maps, etc).
namespace std {
// This is so that we have reasonable hashing of pairs of nodeids
template <> struct hash<nodepair_t> {
    inline size_t operator()(const nodepair_t &v) const {
        return (v.first << 12) + v.second;
    }
};

}

/**
 * The handler for libosmium.  This class basically contains one callback that's called by
 * libosmium as it parses our OSM file.  The `way` function adds data to the class member
 * variables as ways are discovered.
 * This object can then be used to lookup nodes and way tags.
 */
struct Extractor final : osmium::handler::Handler {


    /**
     * The public data structures, this is where everything gets indexed.
     */
    std::unordered_map<nodepair_t, wayid_t> pair_way_map;
    std::unordered_map<wayid_t, tagmap_t> way_tag_map;
    std::unique_ptr<boost::geometry::index::rtree<value_t, boost::geometry::index::rstar<8>>> rtree;

    /**
     * Handler for ways as they are seen
     */
    void way(const osmium::Way& way) {

        // TODO: check if way is interesting (road, highway, etc), there's no need to keep
        // everything....
        const char *oneway = way.tags().get_value_by_key("one_way");
        bool forward = (!oneway || std::strcmp(oneway, "yes") == 0);
        bool reverse = (!oneway || std::strcmp(oneway, "-1") == 0);

        if (way.nodes().size() > 1 && (forward || reverse))
        {
            // This iterates over each pair of nodes.
            // Given the nodes 1,2,3,4,5,6
            // This loop will be called with (1,2), (2,3), (3,4), (4,5), (5, 6)
            std::for_each(
                boost::make_zip_iterator(boost::make_tuple(way.nodes().cbegin(), way.nodes().cbegin()+1)),
                boost::make_zip_iterator(boost::make_tuple(way.nodes().cend()-1, way.nodes().cend())),
                [this,&way,&forward,&reverse](boost::tuple<const osmium::NodeRef, const osmium::NodeRef> pair) {
                    try {
                        point_t a{pair.get<0>().location().lon(), pair.get<0>().location().lat()};
                        point_t b{pair.get<1>().location().lon(), pair.get<1>().location().lat()};
                        if (used_nodes.find(pair.get<0>().ref()) == used_nodes.end())
                        {
                            used_nodes_list.emplace_back(a, pair.get<0>().ref());
                            used_nodes.emplace(pair.get<0>().ref());
                        }
                        if (used_nodes.find(pair.get<1>().ref()) == used_nodes.end())
                        {
                            used_nodes_list.emplace_back(b, pair.get<1>().ref());
                            used_nodes.emplace(pair.get<1>().ref());
                        }
                        if (forward) {
                            pair_way_map.emplace(std::make_pair(pair.get<0>().ref(), pair.get<1>().ref()),way.id());
                        }
                        if (reverse) {
                            pair_way_map.emplace(std::make_pair(pair.get<1>().ref(), pair.get<0>().ref()),way.id());
                        }
                    }
                    catch (const osmium::invalid_location &e)
                    {
                        //std::cerr << "WARNING: Invalid location for one of nodes " << pair.get<0>().ref() << " or " << pair.get<1>().ref() << "\n";
                    }
                }
            );

            // Create a map of the tags for this way, add the strings to the stringbuffer
            // and then add the tag map to the way map.
            tagmap_t tmp;
            for (const osmium::Tag& tag : way.tags()) {
                const auto key_pos = addstring(tag.key());
                const auto val_pos = addstring(tag.value());
                tmp.emplace(key_pos, val_pos);
            }
            way_tag_map.emplace(way.id(),tmp);
        }
    }

    /**
     * This needs to be called once all ways are processed.  It constructs the RTree object using the accumulated data,
     * and frees the memory for the no-longer-needed string index and used nodes list.
     */
    void finish()
    {
        rtree = std::make_unique<boost::geometry::index::rtree<value_t, boost::geometry::index::rstar<8>>>(used_nodes_list.begin(), used_nodes_list.end());

        // Tricks to free memory, swap out data with empty versions
        // This frees the memory.  shrink_to_fit doesn't guarantee that.
        std::unordered_set<nodeid_t>().swap(used_nodes);
        std::vector<value_t>().swap(used_nodes_list);
        std::unordered_map<std::string, std::size_t>().swap(string_index);

        // Hint that these data structures can be shrunk.
        string_data.shrink_to_fit();
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

    private:
    std::unordered_set<nodeid_t> used_nodes;
    std::vector<value_t> used_nodes_list;
    std::unordered_map<std::string, std::size_t> string_index;

    std::vector<char> string_data;
    std::vector<std::pair<std::size_t, std::size_t>> string_offsets;

    // Adds a string to the data blob, returns its index
    std::size_t addstring(const char * str)
    {
        auto idx = string_index.find(str);
        if (idx == string_index.end())
        {
            string_index.emplace(str, string_index.size());
            auto string_length = std::min<std::size_t>(255, std::strlen(str));
            std::copy(str, str + string_length, std::back_inserter(string_data));
            string_offsets.emplace_back(string_data.size() - string_length, string_length);
            idx = string_index.find(str);
        }
        return idx->second;
    }



};

// Fulfilled promise signals clean shutdown
static std::promise<void> signal_shutdown;

int main(int argc, char** argv) try {
  if (argc != 3) {
    std::fprintf(stderr, "Usage: %s uri <filename.[pbf|osm|osm.bz2|osm.gz]>\n", argv[0]);
    return EXIT_FAILURE;
  }

#ifdef __unix__
  // Register CTRL+C handler on POSIX systems for clean shutdown
  struct ::sigaction action;
  ::sigemptyset(&action.sa_mask);
  action.sa_flags = SA_SIGINFO;
  action.sa_sigaction = [](int, ::siginfo_t*, void*) { signal_shutdown.set_value(); };

  if (::sigaction(SIGINT, &action, nullptr) == -1)
    throw std::system_error{errno, std::system_category()};
#endif

  // This is our callback handler, it ends up with all the data
  Extractor extractor;

  // Load OSM file using libosmium
  // Do this in a block so that libosmium gets cleaned up with RAII
  {
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
  }
  std::cerr << "done\n";
  std::cerr << "Number of node pairs indexed: " << extractor.pair_way_map.size() << "\n";
  std::cerr << "Number of ways indexed: " << extractor.way_tag_map.size() << "\n";
  std::cerr << "Constructing RTree ... " << std::flush;
  // This constructs the rtree, and releases some no-longer needed data once the rtree is built
  extractor.finish();
  std::cerr << "done\n" << std::flush;

  // Setup the web server
  // Server listens based on URI (Host, Port)
  web::uri uri{argv[1]};
  web::http::experimental::listener::http_listener listener{uri};

  // GET Request Handler
  listener.support(web::http::methods::GET, [&extractor](const auto& request) {
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
    std::vector<nodeid_t> nodeids;
    std::vector<double> coordinates;

    // If /nodelist fails, we'll try /coordlist
    if (!parse(begin(path), end(path), nodelist_parser, nodeids) || nodeids.size() < 2) {

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
            extractor.rtree->query(boost::geometry::index::nearest(pt, 1), std::back_inserter(results));

            // If we got exactly one hit (we should), append it to our nodeids list, if it was within 1m
            if (results.size() == 1 && boost::geometry::distance(pt, results[0].first) < 1)
            {
                nodeids.push_back(results[0].second);
            } 
            // otherwise, insert a 0 value, this coordinate didn't match
            else 
            {
                nodeids.push_back(0);
            }
        }

    }

    // If after all of the above, we still don't have at least two nodes, this is a bad request
    if (nodeids.size() < 2)
    {
      web::json::value response;
      response["message"] = web::json::value("Bad request");
      request.reply(web::http::status_codes::BadRequest,response);
    }

    web::json::value response;

    std::vector<wayid_t> wayid_list;
    std::unordered_set<wayid_t> wayid_set;

    // the zip_iterator allows us to group pairs.  So, if we get 1,2,3,4
    // in our nodeids vector, this for_each will loop over:
    //   (1,2) (2,3) (3,4)
    std::for_each(
        boost::make_zip_iterator(boost::make_tuple(nodeids.cbegin(), nodeids.cbegin()+1)),
        boost::make_zip_iterator(boost::make_tuple(nodeids.cend()-1, nodeids.cend())),
        [&extractor,&wayid_list,&wayid_set](boost::tuple<const nodeid_t, const nodeid_t> pair) {
            // If our pair exists in the pair_way_map
            if (extractor.pair_way_map.find(std::make_pair(pair.get<0>(), pair.get<1>())) != extractor.pair_way_map.end())
            {
                // Get the way id for this node pair
                const auto wayid = extractor.pair_way_map[std::make_pair(pair.get<0>(), pair.get<1>())];
                wayid_list.push_back(wayid);
                wayid_set.insert(wayid);
            }
            else
            {
                wayid_list.push_back(0);
            }
        }
    );

    web::json::value j_array = web::json::value::array(wayid_list.size());
    int idx = 0;
    for (const auto &wayid : wayid_list) {
        j_array[idx++] = wayid;
    }

    response["ways_touched"] = j_array;

    web::json::value waydata;

    for (const auto &wayid : wayid_set)
    {
        if (wayid == 0) continue;
        // Then get the tags for that way
        const auto tagmap = extractor.way_tag_map[wayid];

        // an create the tag key/val dictionary for that
        web::json::value tags = web::json::value::object();
        for (const auto &entry : tagmap) {
            tags[extractor.getstring(entry.first)] = web::json::value(extractor.getstring(entry.second));
        }
        waydata[std::to_string(wayid)] = tags;
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
