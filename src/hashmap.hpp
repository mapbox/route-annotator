#include <fstream>
#include <iostream>
#include "types.hpp"
#include <sparsepp/spp.h>

using spp::sparse_hash_map;

struct Way 
{
    bool operator==(const Way &o) const 
    { return to == o.to && from == o.from; }

    Way(int to, int from) : to(to), from(from) { }

    int to;
    int from;
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
            std::size_t seed = 0;
            spp::hash_combine(seed, p.to);
            spp::hash_combine(seed, p.from);
            return seed;
        }
    };
}

class Hashmap {
    // TODO: turn into private
    public:
        void add(external_nodeid_t to, external_nodeid_t from, speed_t speed);

    public:
        void loadData(std::ifstream input);
        speed_t getValue(external_nodeid_t to, external_nodeid_t from);
        bool hasKey(external_nodeid_t to, external_nodeid_t from);

    private:
        sparse_hash_map<Way, int> annotations;
};