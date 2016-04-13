#pragma once

#include "types.hpp"

#include <boost/geometry/index/rtree.hpp>

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
