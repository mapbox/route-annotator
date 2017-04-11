#include "hashmap.hpp"
#include <sparsepp/spp.h>

using spp::sparse_hash_map;

void Hashmap::add(external_nodeid_t to, external_nodeid_t from, speed_t speed) {
    Hashmap::annotations[Way(to, from)] = speed;
};

bool Hashmap::hasKey(external_nodeid_t to, external_nodeid_t from) {

    auto lookup = annotations.find(Way(to,from));
    
    if(lookup != annotations.end()) {
        return true;
    } else {
        return false;
    }
};

void Hashmap::loadData(std::ifstream input) {};

speed_t Hashmap::getValue(external_nodeid_t to, external_nodeid_t from){

    if (!Hashmap::hasKey(to, from)) {
        throw std::runtime_error("Way from NodeID " + std::to_string(to) + "to NodeId " + std::to_string(from) + " doesn't exist in the hashmap.");
    } 

    return annotations[Way(to, from)];
};