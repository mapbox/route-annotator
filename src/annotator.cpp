#include "annotator.hpp"
#include "extractor.hpp"

#include <cstddef>

// For boost RTree
#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>

#include <boost/geometry/strategies/spherical/distance_haversine.hpp>
//#include <boost/log/trivial.hpp>

RouteAnnotator::RouteAnnotator(const Database &db) : db(db) {}

std::vector<internal_nodeid_t>
RouteAnnotator::coordinates_to_internal(const std::vector<point_t> &points)
{

    static const boost::geometry::strategy::distance::haversine<double> haversine(6372795.0);
    static const double MAX_DISTANCE = 5;

    if (!db.rtree)
        throw std::runtime_error("RTree is null - need to call compact() on database before use");

    std::vector<internal_nodeid_t> internal_nodeids;
    for (const auto &point : points)
    {
        std::vector<value_t> rtree_results;

        // Search for the nearest point to the one supplied
        db.rtree->query(boost::geometry::index::nearest(point, 1),
                        std::back_inserter(rtree_results));

        // If we got exactly one hit (we should), append it to our nodeids list, if it
        // was within 1m
        /*
        BOOST_LOG_TRIVIAL(debug) << "Distance from " << point.get<0>() << "," << point.get<1>()
                                 << " to  " << rtree_results[0].first.get<0>() << ","
                                 << rtree_results[0].first.get<1>() << " is "
                                 << boost::geometry::distance(point, rtree_results[0].first,
                                                              haversine)
                                 << "\n";
        */
        if (rtree_results.size() == 1 &&
            boost::geometry::distance(point, rtree_results[0].first, haversine) < MAX_DISTANCE)
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
RouteAnnotator::external_to_internal(const std::vector<external_nodeid_t> &external_nodeids)
{
    // Convert external node ids into internal ones
    std::vector<internal_nodeid_t> results;
    std::for_each(external_nodeids.begin(), external_nodeids.end(),
                  [this, &results](const external_nodeid_t n) {
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

annotated_route_t RouteAnnotator::annotateRoute(const std::vector<internal_nodeid_t> &route)
{
    annotated_route_t result;

    for (std::size_t i = 0; i < route.size() - 1; i++)
    {
        const auto way_id = db.pair_way_map.find(std::make_pair(route[i], route[i + 1]));
        if (way_id != db.pair_way_map.end())
        {
            result.push_back(way_id->second);
        }
        else
        {
            result.push_back(INVALID_WAYID);
        }
    }
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

tagrange_t RouteAnnotator::get_tag_range(const wayid_t way_id) { return db.way_tag_ranges[way_id]; }
