#include "hashmap.hpp"
#include <sparsepp/spp.h>

using spp::sparse_hash_map;

void Hashmap::add(external_nodeid_t to, external_nodeid_t from, congestion_speed_t speed) {
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

void Hashmap::loadData(std::ifstream& input) {
    std::string line;
    external_nodeid_t to;
    external_nodeid_t from;
    congestion_speed_t speed;
    std::string str_to;
    std::string str_from;
    std::string str_speed;

    if (input)
    {
        while (getline(input, line))
        {
            try
            {
                std::stringstream iss;
                iss << line;
                std::getline(iss, str_to, ',');
                to = std::stoull(str_to);
                std::getline(iss, str_from, ',');
                from = std::stoull(str_from);
                std::getline(iss, str_speed, ',');
                speed = std::stoull(str_speed);

                add(to, from, speed);
            }
            catch (std::exception& e)
            {
                std::cout << "Input input has invalid format." << std::endl;
                std::cout << e.what() << std::endl;
            }
        }
    }
    input.close();
};

congestion_speed_t Hashmap::getValue(external_nodeid_t to, external_nodeid_t from){

    if (!Hashmap::hasKey(to, from)) {
        throw std::runtime_error("Way from NodeID " + std::to_string(to) + "to NodeId " + std::to_string(from) + " doesn't exist in the hashmap.");
    }

    return annotations[Way(to, from)];
};