#ifndef ROUTE_ANNOTATOR_HASHMAP_H
#define ROUTE_ANNOTATOR_HASHMAP_H

#include <fstream>
#include <iostream>
#include <sparsepp/spp.h>

#include "types.hpp"

using spp::sparse_hash_map;

struct Way
{
    bool operator==(const Way &o) const { return to == o.to && from == o.from; }

    Way(const external_nodeid_t from, const external_nodeid_t to) : from(from), to(to) {}

    external_nodeid_t from;
    external_nodeid_t to;
};

namespace std
{
// inject specialization of std::hash for Way into namespace std
// ----------------------------------------------------------------
template <> struct hash<Way>
{
    std::size_t operator()(Way const &p) const
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

class Hashmap
{
  public:
    Hashmap();
    Hashmap(const std::string &input_filename);
    inline void add(const external_nodeid_t &from,
                    const external_nodeid_t &to,
                    const congestion_speed_t &speed);
    bool hasKey(const external_nodeid_t &from, const external_nodeid_t &to) const;
    congestion_speed_t getValue(const external_nodeid_t &from, const external_nodeid_t &to) const;
    std::vector<congestion_speed_t> getValues(const std::vector<external_nodeid_t> &way) const;

  private:
    sparse_hash_map<Way, congestion_speed_t> annotations;
};

#endif