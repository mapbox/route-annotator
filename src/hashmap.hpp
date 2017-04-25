#include <fstream>
#include <iostream>
#include <sparsepp/spp.h>

#include "types.hpp"
#include <sparsepp/spp.h>

using spp::sparse_hash_map;

struct Way
{
    bool operator==(const Way &o) const
    {
        return to == o.to && from == o.from;
    }

    Way(const external_nodeid_t to, const external_nodeid_t from) : to(to), from(from) { }

    external_nodeid_t to;
    external_nodeid_t from;
};

namespace std
{
    // inject specialization of std::hash for Way into namespace std
    // ----------------------------------------------------------------
    template<>
    struct hash<Way>
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

class Hashmap {
    public:
        Hashmap();
        // Hashmap(std::ifstream& input);
        Hashmap(const std::string &input_filename);
        inline void add(const external_nodeid_t &to, const external_nodeid_t &from, const congestion_speed_t &speed);
        congestion_speed_t getValue(external_nodeid_t to, external_nodeid_t from) const;
        bool hasKey(external_nodeid_t to, external_nodeid_t from) const;
        std::vector<congestion_speed_t> getValues(std::vector<external_nodeid_t>& way) const;

    private:
        sparse_hash_map<Way, int> annotations;
};
