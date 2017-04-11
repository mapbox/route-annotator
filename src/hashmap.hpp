#include <fstream>
#include <iostream>
#include "types.hpp"

class Hashmap {
    // TODO: turn into private
    public:
        void add(external_nodeid_t to, external_nodeid_t from, speed_t speed);
        bool hasKey(external_nodeid_t to, external_nodeid_t from);

    public:
        void loadData(std::ifstream input);
        speed_t getValue(external_nodeid_t to, external_nodeid_t from);
};