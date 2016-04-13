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

static constexpr internal_nodeid_t INVALID_INTERNAL_NODEID = std::numeric_limits<internal_nodeid_t>::max();

// Node indexing types for libosmium
// We need these because we need access to the lon/lat for the noderefs inside the way
// callback in our handler.
typedef osmium::index::map::Dummy<osmium::unsigned_object_id_type, osmium::Location> index_neg_type;
typedef osmium::index::map::SparseMemArray<osmium::unsigned_object_id_type, osmium::Location>
    index_pos_type;
// typedef osmium::index::map::DenseMmapArray<osmium::unsigned_object_id_type, osmium::Location>
// index_pos_type;
typedef osmium::handler::NodeLocationsForWays<index_pos_type, index_neg_type> location_handler_type;

// Data types for the RTree
// typedef boost::geometry::model::point<double, 2,
// boost::geometry::cs::geographic<boost::geometry::degree>> point_t;
typedef boost::geometry::model::
    point<double, 2, boost::geometry::cs::spherical_equatorial<boost::geometry::degree>> point_t;
// typedef boost::geometry::model::point<double, 2, boost::geometry::cs::cartesian> point_t;
typedef std::pair<point_t, internal_nodeid_t> value_t;

// Data types for our lookup tables
typedef std::pair<internal_nodeid_t, internal_nodeid_t> internal_nodepair_t;

typedef std::unordered_map<external_nodeid_t, internal_nodeid_t> external_internal_map_t;

// Our tagmap is a set of indexes.  We don't store the strings themselves, we store
// indexes into our string lookup tables, which only stores each unique string once.
typedef std::vector<std::pair<std::uint32_t, std::uint32_t>> uint32_pairvector_t;

// Custom hashing functions for our data types to use in standard containers (sets, maps, etc).
namespace std
{
// This is so that we have reasonable hashing of pairs of nodeids
template <> struct hash<internal_nodepair_t>
{
    inline size_t operator()(const internal_nodepair_t &v) const
    {
        return (static_cast<std::uint64_t>(v.first) << 32) + v.second;
    }
};
}

struct Database {
    std::unordered_map<internal_nodepair_t, tagsetid_t> pair_tagset_map;
    uint32_pairvector_t tagset_list;
    uint32_pairvector_t key_value_pairs;
    std::unique_ptr<boost::geometry::index::rtree<value_t, boost::geometry::index::rstar<8>>> rtree;
    std::unordered_map<external_nodeid_t, internal_nodeid_t> external_internal_map;
    // Private
    std::unordered_map<std::string, std::uint32_t> string_index;
    std::vector<value_t> used_nodes_list;

    std::vector<char> string_data;
    uint32_pairvector_t string_offsets;
    std::uint32_t addstring(const char *str);
    std::string getstring(const std::size_t stringid);

    void compact();
    void dump();
};

struct AnnotatedRoute
{
    std::vector<int> tagset_offset_list;
    std::unordered_map<tagsetid_t, int> tagset_indexes;
    std::vector<tagsetid_t> tagset_list;
};

// Forward declaration
struct RouteAnnotator
{
    public:
        RouteAnnotator(const std::string &osmfilename);

        std::vector<internal_nodeid_t> coordinates_to_internal(const std::vector<point_t> & points);
        std::vector<internal_nodeid_t> external_to_internal(const std::vector<external_nodeid_t> & external_nodeids);

        AnnotatedRoute annotateRoute(const std::vector<internal_nodeid_t> & route);

        std::string get_tag_key(const std::size_t index);
        std::string get_tag_value(const std::size_t index);
        std::pair<std::uint32_t, std::uint32_t> get_tag_range(const std::size_t index);

    private:
        Database db;
};


/**
 * The handler for libosmium.  This class basically contains one callback that's called by
 * libosmium as it parses our OSM file.  The `way` function adds data to the class member
 * variables as ways are discovered.
 * This object can then be used to lookup nodes and way tags.
 */
struct Extractor final : osmium::handler::Handler
{

    Database &db;

    Extractor(Database & d) : db(d) { }

    /**
     * Handler for ways as they are seen
     */
    void way(const osmium::Way &way)
    {

        // TODO: check if way is interesting (road, highway, etc), there's no need to keep
        // everything....
        const char *oneway = way.tags().get_value_by_key("one_way");
        bool forward = (!oneway || std::strcmp(oneway, "yes") == 0);
        bool reverse = (!oneway || std::strcmp(oneway, "-1") == 0);

        const char *highway = way.tags().get_value_by_key("highway");
        bool usable =
            highway &&
            (std::strcmp(highway, "motorway") == 0 || std::strcmp(highway, "motorway_link") == 0 ||
             std::strcmp(highway, "trunk") == 0 || std::strcmp(highway, "trunk_link") == 0 ||
             std::strcmp(highway, "primary") == 0 || std::strcmp(highway, "primary_link") == 0 ||
             std::strcmp(highway, "secondary") == 0 ||
             std::strcmp(highway, "secondary_link") == 0 || std::strcmp(highway, "tertiary") == 0 ||
             std::strcmp(highway, "tertiary_link") == 0 ||
             std::strcmp(highway, "residential") == 0 ||
             std::strcmp(highway, "living_street") == 0 ||
             std::strcmp(highway, "unclassified") == 0 || std::strcmp(highway, "service") == 0 ||
             std::strcmp(highway, "ferry") == 0 || std::strcmp(highway, "movable") == 0 ||
             std::strcmp(highway, "shuttle_train") == 0 || std::strcmp(highway, "default") == 0);

        if (usable && way.nodes().size() > 1 && (forward || reverse))
        {

            BOOST_ASSERT(db.key_value_pairs.size() < std::numeric_limits<std::uint32_t>::max());
            const auto tagstart = static_cast<std::uint32_t>(db.key_value_pairs.size());
            // Create a map of the tags for this way, add the strings to the stringbuffer
            // and then add the tag map to the way map.
            for (const osmium::Tag &tag : way.tags())
            {
                const auto key_pos = db.addstring(tag.key());
                const auto val_pos = db.addstring(tag.value());
                db.key_value_pairs.emplace_back(key_pos, val_pos);
            }
            db.key_value_pairs.emplace_back(db.addstring("_way_id"),
                                            db.addstring(std::to_string(way.id()).c_str()));
            BOOST_ASSERT(db.key_value_pairs.size() < std::numeric_limits<std::uint32_t>::max());
            const auto tagend = static_cast<std::uint32_t>(db.key_value_pairs.size() - 1);
            db.tagset_list.emplace_back(tagstart, tagend);

            BOOST_ASSERT(db.tagset_list.size() < std::numeric_limits<tagsetid_t>::max());
            const auto tagset_id = static_cast<tagsetid_t>(db.tagset_list.size() - 1);

            // This iterates over each pair of nodes.
            // Given the nodes 1,2,3,4,5,6
            // This loop will be called with (1,2), (2,3), (3,4), (4,5), (5, 6)
            std::for_each(
                boost::make_zip_iterator(
                    boost::make_tuple(way.nodes().cbegin(), way.nodes().cbegin() + 1)),
                boost::make_zip_iterator(
                    boost::make_tuple(way.nodes().cend() - 1, way.nodes().cend())),
                [this, tagset_id, forward,
                 reverse](boost::tuple<const osmium::NodeRef, const osmium::NodeRef> pair)
                {
                    try
                    {
                        point_t a{pair.get<0>().location().lon(), pair.get<0>().location().lat()};
                        point_t b{pair.get<1>().location().lon(), pair.get<1>().location().lat()};

                        internal_nodeid_t internal_a_id = 0;
                        internal_nodeid_t internal_b_id = 0;

                        const auto tmp_a = db.external_internal_map.find(pair.get<0>().ref());

                        if (tmp_a == db.external_internal_map.end())
                        {
                            internal_a_id =
                                static_cast<internal_nodeid_t>(db.used_nodes_list.size());
                            db.used_nodes_list.emplace_back(a, internal_a_id);
                            db.external_internal_map.emplace(pair.get<0>().ref(), internal_a_id);
                        }
                        else
                        {
                            internal_a_id = tmp_a->second;
                        }

                        const auto tmp_b = db.external_internal_map.find(pair.get<1>().ref());
                        if (tmp_b == db.external_internal_map.end())
                        {
                            internal_b_id =
                                static_cast<internal_nodeid_t>(db.used_nodes_list.size());
                            db.used_nodes_list.emplace_back(b, internal_b_id);
                            db.external_internal_map.emplace(pair.get<1>().ref(), internal_b_id);
                        }
                        else
                        {
                            internal_b_id = tmp_b->second;
                        }

                        if (forward)
                        {
                            db.pair_tagset_map.emplace(std::make_pair(internal_a_id, internal_b_id),
                                                       tagset_id);
                        }
                        if (reverse)
                        {
                            db.pair_tagset_map.emplace(std::make_pair(internal_b_id, internal_a_id),
                                                       tagset_id);
                        }
                    }
                    catch (const osmium::invalid_location &e)
                    {
                        // std::cerr << "WARNING: Invalid location for one of nodes " <<
                        // pair.get<0>().ref() << " or " << pair.get<1>().ref() << "\n";
                    }
                });
        }
    }
};


RouteAnnotator::RouteAnnotator(const std::string &osmfilename)
{
    Extractor extractor(db);
    std::cerr << "Parsing " << osmfilename << " ... " << std::flush;
    osmium::io::File osmfile{osmfilename};
    osmium::io::Reader fileReader(osmfile,
                                  osmium::osm_entity_bits::way | osmium::osm_entity_bits::node);
    int fd = open("nodes.cache", O_RDWR | O_CREAT, 0666);
    if (fd == -1)
    {
        throw std::runtime_error(strerror(errno));
    }
    index_pos_type index_pos{fd};
    index_neg_type index_neg;
    location_handler_type location_handler(index_pos, index_neg);
    location_handler.ignore_errors();
    osmium::apply(fileReader, location_handler, extractor);
    std::cerr << "done\n";
    std::cerr << "Number of node pairs indexed: " << db.pair_tagset_map.size() << "\n";
    std::cerr << "Number of ways indexed: " << db.tagset_list.size() << "\n";

    std::cerr << "Constructing RTree ... " << std::flush;
    db.compact();
    std::cerr << "done\n" << std::flush;
    db.dump();
}

/**
 * This needs to be called once all ways are processed.  It constructs the RTree object using the
 * accumulated data,
 * and frees the memory for the no-longer-needed string index and used nodes list.
 */
void Database::compact()
{
    rtree = std::make_unique<
        boost::geometry::index::rtree<value_t, boost::geometry::index::rstar<8>>>(
        used_nodes_list.begin(), used_nodes_list.end());

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
std::string Database::getstring(const std::size_t stringid)
{
    auto stringinfo = string_offsets[stringid];
    std::string result(string_data.begin() + stringinfo.first,
                       string_data.begin() + stringinfo.first + stringinfo.second);
    return result;
}

// Adds a string to the data blob, returns its index
std::uint32_t Database::addstring(const char *str)
{
    auto idx = string_index.find(str);
    if (idx == string_index.end())
    {
        BOOST_ASSERT(string_index.size() < std::numeric_limits<std::uint32_t>::max());
        string_index.emplace(str, static_cast<uint32_t>(string_index.size()));
        auto string_length =
            static_cast<std::uint32_t>(std::min<std::size_t>(255, std::strlen(str)));
        std::copy(str, str + string_length, std::back_inserter(string_data));
        BOOST_ASSERT(string_data.size() < std::numeric_limits<std::uint32_t>::max());
        BOOST_ASSERT(string_data.size() >= string_length);
        string_offsets.emplace_back(
            static_cast<std::uint32_t>(string_data.size()) - string_length, string_length);
        idx = string_index.find(str);
    }
    BOOST_ASSERT(idx->second < std::numeric_limits<std::uint32_t>::max());
    return static_cast<std::uint32_t>(idx->second);
}

void Database::dump()
{
    std::cout << "String data is " << (string_data.capacity() * sizeof(char))
              << " Used: " << (string_data.size() * sizeof(char)) << "\n";
    std::cout << "tagset = Allocated "
              << (tagset_list.capacity() * sizeof(uint32_pairvector_t::value_type))
              << "  Used: " << (tagset_list.size() * sizeof(uint32_pairvector_t::value_type))
              << "\n";
    std::cout << "keyvalue = Allocated "
              << (key_value_pairs.capacity() * sizeof(uint32_pairvector_t::value_type))
              << "  Used: "
              << (key_value_pairs.size() * sizeof(uint32_pairvector_t::value_type)) << "\n";
    std::cout << "stringoffset = Allocated "
              << (string_offsets.capacity() * sizeof(uint32_pairvector_t::value_type))
              << "  Used: " << (string_offsets.size() * sizeof(uint32_pairvector_t::value_type))
              << "\n";

    std::cout << "pair_tagset_map = Allocated "
              << (pair_tagset_map.size() *
                  (sizeof(uint32_pairvector_t::value_type) + sizeof(tagsetid_t)))
              << "  Load factor: " << pair_tagset_map.load_factor()
              << " Buckets: " << pair_tagset_map.bucket_count() << "\n";
}

std::vector<internal_nodeid_t> 
RouteAnnotator::coordinates_to_internal(const std::vector<point_t> & points)
{
    std::vector<internal_nodeid_t> internal_nodeids;
    for (const auto & point : points)
    {
        std::vector<value_t> rtree_results;

        // Search for the nearest point to the one supplied
        db.rtree->query(boost::geometry::index::nearest(point, 1), std::back_inserter(rtree_results));

        // If we got exactly one hit (we should), append it to our nodeids list, if it
        // was within 1m
        // TODO: verify that we're actually testing 1m accuracy here
        if (rtree_results.size() == 1 && boost::geometry::distance(point, rtree_results[0].first) < 1)
        {
            internal_nodeids.push_back(rtree_results[0].second);
        }
        // otherwise, insert a 0 value, this coordinate didn't match
        else
        {
            internal_nodeids.push_back(INVALID_INTERNAL_NODEID);
        }
    }
    return internal_nodeids;
}

std::vector<internal_nodeid_t>
RouteAnnotator::external_to_internal(const std::vector<external_nodeid_t> & external_nodeids)
{
    // Convert external node ids into internal ones
    std::vector<internal_nodeid_t> results;
    std::for_each(external_nodeids.begin(), external_nodeids.end(),
        [this, &results](const external_nodeid_t n)
        {
            const auto internal_node_id = db.external_internal_map.find(n);
            if (internal_node_id == db.external_internal_map.end())
            {
                // Push an invalid nodeid into the list if we didn't match
                results.push_back(INVALID_INTERNAL_NODEID);
            }
            else
            {
                results.push_back(internal_node_id->second);
            }
    });
    return results;
}

AnnotatedRoute
RouteAnnotator::annotateRoute(const std::vector<internal_nodeid_t> & route)
{
    AnnotatedRoute result;
    // the zip_iterator allows us to group pairs.  So, if we get 1,2,3,4
    // in our nodeids vector, this for_each will loop over:
    //   (1,2) (2,3) (3,4)
    std::for_each(
        boost::make_zip_iterator(
            boost::make_tuple(route.cbegin(), route.cbegin() + 1)),
        boost::make_zip_iterator(
            boost::make_tuple(route.cend() - 1, route.cend())),
        [this, &result](boost::tuple<const internal_nodeid_t, const internal_nodeid_t> pair)
        {
            // If our pair exists in the pair_tagset_map
            const auto tagset_id =
                 db.pair_tagset_map.find(std::make_pair(pair.get<0>(), pair.get<1>()));
            if (tagset_id != db.pair_tagset_map.end())
            {
                if (result.tagset_indexes.find(tagset_id->second) == result.tagset_indexes.end())
                {
                    result.tagset_list.push_back(tagset_id->second);
                    result.tagset_indexes[tagset_id->second] =
                        static_cast<int>(result.tagset_list.size() - 1);
                }
                result.tagset_offset_list.push_back(result.tagset_indexes.find(tagset_id->second)->second);
            }
            else
            {
                result.tagset_offset_list.push_back(-1);
            }
    });
    return result;
}

std::string RouteAnnotator::get_tag_key(const std::size_t index)
{
    return db.getstring(db.key_value_pairs[index].first);
}

std::string RouteAnnotator::get_tag_value(const std::size_t index)
{
    return db.getstring(db.key_value_pairs[index].second);
}

std::pair<std::uint32_t, std::uint32_t> 
RouteAnnotator::get_tag_range(const std::size_t index)
{
    return db.tagset_list[index];
}
