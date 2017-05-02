#include "hashmap.hpp"
#include "csv.hpp"
#include <sparsepp/spp.h>

using spp::sparse_hash_map;

Hashmap::Hashmap() {};

Hashmap::Hashmap(const std::string &input_filename) {

    io::CSVReader<3> in(input_filename);
    in.set_header("from", "to", "speed");
    external_nodeid_t from; external_nodeid_t to; congestion_speed_t speed;
    // Pre-allocate a large chunk of memory to save on all the micro-allocations
    // that would happen if adding items one-by-one and growing as needed
    // This is the size of the wholeworld.csv file that contains all of the freeflow data currently (as of May 2, 2017)
    annotations.reserve(139064548);
    while(in.read_row(from, to, speed)){
        add(from, to, speed);
    }

};

void Hashmap::add(const external_nodeid_t &from, const external_nodeid_t &to, const congestion_speed_t &speed) {
    annotations[Way(from,to)] = speed;
};

bool Hashmap::hasKey(const external_nodeid_t &from, const external_nodeid_t &to) const {
    return annotations.count(Way(from,to)) > 0;
};

congestion_speed_t Hashmap::getValue(const external_nodeid_t &from, const external_nodeid_t &to) const
{
    // Save the result of find so that we don't need to repeat the lookup to get the value
    auto result = annotations.find(Way(from, to));
    if (result == annotations.end()) {
        throw std::runtime_error("Way from NodeID " + std::to_string(from) + " to NodeId " + std::to_string(to) + " doesn't exist in the hashmap.");
    }

    // Use the already retrieved value as the result
    return result->second;
};

std::vector<congestion_speed_t> Hashmap::getValues(const std::vector<external_nodeid_t> &way) const
{
    std::vector<congestion_speed_t> speeds;
    if (way.size() < 2) {
        throw std::runtime_error("NodeID Array should have more than 2 NodeIds for getValues methodr.");
    }

    if (way.size() > 1)
    {
        speeds.resize(way.size() - 1);
        for (std::size_t node_id = 0; node_id < speeds.size(); node_id++)
        {
            if (hasKey(way[node_id], way[node_id+1]))
            {
                speeds[node_id] = getValue(way[node_id], way[node_id+1]);
            }
            else
            {
                speeds[node_id] = INVALID_SPEED;
            }
        }
    }
    return speeds;
};
