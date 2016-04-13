#pragma once
#include <string>
#include <utility>
#include <vector>

#include "database.hpp"
#include "types.hpp"

/**
 * This is the wrapper object for the route annotator.  It presents a simple
 * API for getting tag information back from a sequence of OSM nodes, or
 * real coordinates.
 */
struct RouteAnnotator
{
  public:
    /**
     * Constructs a route annotator.
     *
     * @param osmfilename the name of the OSM file to load
     */
    RouteAnnotator(const std::string &osmfilename);

    /**
     * Convert a list of lon/lat coordinates into internal
     * node ids.
     *
     * @param points a vector of lon/lat coordinates
     * @return a vector of internal node ids.  Will return INVALID_INTERNAL_NODEID
     *     for coordinates that couldn't be matched.
     */
    std::vector<internal_nodeid_t> 
        coordinates_to_internal(const std::vector<point_t> &points);

    /**
     * Convert OSM node ids (64bit) into internal node ids (32 bit).
     *
     * @param external_nodeids the list of OSM nodeids for the route
     * @return a vector of internal node ids.  Will return INVALID_INTERNAL_NODEID
     *     for coordinates that couldn't be matched.
     */
    std::vector<internal_nodeid_t>
        external_to_internal(const std::vector<external_nodeid_t> &external_nodeids);

    /**
     * Gets the tags for a route
     *
     * @param route a list of connected internal node ids
     * @return a vector of way ids that each pair of nodes on the route touches,
     *     or INVALID_WAYID values if there was no way found for a pair of nodes.
     */
    annotated_route_t annotateRoute(const std::vector<internal_nodeid_t> &route);

    /**
     * Gets the key part for a tag
     *
     * @param index the index for the tag
     * @return the actual key name as a string
     */
    std::string get_tag_key(const std::size_t index);

    /**
     * Gets the value part of a tag
     *
     * @param index the index for the tag
     * @return the value of the tag as a string
     */
    std::string get_tag_value(const std::size_t index);

    /**
     * Gets the start and end indexes for the tags for a way.
     * You can iterate over the values between these two
     * indexes to get all the key/value pairs.
     *
     * @param wayid the way id to get the list of tags for
     * @return the start and end indexes for the tags for this way
     */
    tagrange_t get_tag_range(const wayid_t wayid);

  private:
    // This is where all the data lives
    Database db;
};
