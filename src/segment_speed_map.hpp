#ifndef SEGMENT_SPEED_MAP_H
#define SEGMENT_SPEED_MAP_H

#include <fstream>
#include <iostream>
#include <sparsepp/spp.h>
#include <vector>

#include "types.hpp"

using spp::sparse_hash_map;

/**
 * This type describes a segment on the map - two nodes
 */
struct Segment
{
    bool operator==(const Segment &o) const { return to == o.to && from == o.from; }

    Segment(const external_nodeid_t from, const external_nodeid_t to) : from(from), to(to) {}

    external_nodeid_t from;
    external_nodeid_t to;
};

namespace std
{
// inject specialization of std::hash for Segment into namespace std
// ----------------------------------------------------------------
template <> struct hash<Segment>
{
    std::size_t operator()(Segment const &p) const
    {
        // As of 2017, we know that OSM node IDs are about 2^32 big.  In a 64bit
        // value, most of the top bits are zero.
        // A quick way to come up with a unique ID is to merge two IDs together,
        // shifting one into the upper empty bits of the other.
        // This produces few/no collisions, which is good for hashtable/map
        // performance.
        return (p.to << 31) + p.from;
    }
};
}

class SegmentSpeedMap
{
  public:
    /**
     * Do-nothing constructor
     */
    SegmentSpeedMap();

    /**
     * Loads from,to,speed data from a single file
     */
    SegmentSpeedMap(const std::string &input_filename);

    /**
     * Loads from,to,speed data from multiple files
     */
    SegmentSpeedMap(const std::vector<std::string> &input_filenames);

    /**
     * Parses and loads another CSV file into the existing data
     */
    void loadCSV(const std::string &input_filename);

    /**
     * Adds a single to/from pair value
     */
    inline void add_with_unit(const external_nodeid_t &from,
                              const external_nodeid_t &to,
                              const segment_speed_t &speed,
                              const bool &mph);

    /**
     * Adds a single to/from pair value
     */
    inline void
    add(const external_nodeid_t &from, const external_nodeid_t &to, const segment_speed_t &speed);

    /**
     * Checks if a pair of nodes exist together in the map
     */
    bool hasKey(const external_nodeid_t &from, const external_nodeid_t &to) const;

    /**
     * Gets the value for a pair of nodes.
     * @throws a runtime_exception if the pair aren't found.
     */
    segment_speed_t getValue(const external_nodeid_t &from, const external_nodeid_t &to) const;

    /**
     * Given a list of nodes, returns the speed for each sequential pair.
     * e.g. given A,B,C,D,E, will return speeds for AB,BC,CD,DE
     * If certain pairs don't exist, the value in the result array will be INVALID_SPEED
     * There should be at least 2 values in in the `route` array
     */
    std::vector<segment_speed_t> getValues(const std::vector<external_nodeid_t> &route) const;

  private:
    sparse_hash_map<Segment, segment_speed_t> annotations;
};

#endif
