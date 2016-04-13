#include "annotator.hpp"
#include "extractor.hpp"

#include <cstdio>

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

// Node indexing types for libosmium
// We need these because we need access to the lon/lat for the noderefs inside the way
// callback in our handler.
typedef osmium::index::map::Dummy<osmium::unsigned_object_id_type, osmium::Location> index_neg_type;
typedef osmium::index::map::SparseMemArray<osmium::unsigned_object_id_type, osmium::Location>
    index_pos_type;
// typedef osmium::index::map::DenseMmapArray<osmium::unsigned_object_id_type, osmium::Location>
// index_pos_type;
typedef osmium::handler::NodeLocationsForWays<index_pos_type, index_neg_type> location_handler_type;

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
    std::unordered_map<tagsetid_t, int> tagset_indexes;
    // the zip_iterator allows us to group pairs.  So, if we get 1,2,3,4
    // in our nodeids vector, this for_each will loop over:
    //   (1,2) (2,3) (3,4)
    std::for_each(
        boost::make_zip_iterator(
            boost::make_tuple(route.cbegin(), route.cbegin() + 1)),
        boost::make_zip_iterator(
            boost::make_tuple(route.cend() - 1, route.cend())),
        [this, &result, &tagset_indexes](boost::tuple<const internal_nodeid_t, const internal_nodeid_t> pair)
        {
            // If our pair exists in the pair_tagset_map
            const auto tagset_id =
                 db.pair_tagset_map.find(std::make_pair(pair.get<0>(), pair.get<1>()));
            if (tagset_id != db.pair_tagset_map.end())
            {
                if (tagset_indexes.find(tagset_id->second) == tagset_indexes.end())
                {
                    result.tagset_list.push_back(tagset_id->second);
                    tagset_indexes[tagset_id->second] =
                        static_cast<int>(result.tagset_list.size() - 1);
                }
                result.tagset_offset_list.push_back(tagset_indexes.find(tagset_id->second)->second);
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
