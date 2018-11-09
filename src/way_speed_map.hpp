#ifndef WAY_SPEED_MAP_H
#define WAY_SPEED_MAP_H

#include <fstream>
#include <iostream>
#include <sparsepp/spp.h>
#include <vector>

#include "types.hpp"

using spp::sparse_hash_map;

class WaySpeedMap
{
  public:
    /**
     * Do-nothing constructor
     */
    WaySpeedMap();

    /**
     * Loads from,to,speed data from a single file
     */
    WaySpeedMap(const std::string &input_filename);

    /**
     * Loads way,speed data from multiple files
     */
    WaySpeedMap(const std::vector<std::string> &input_filenames);

    /**
     * Parses and loads another CSV file into the existing data
     */
    void loadCSV(const std::string &input_filename);

    /**
     * Adds a single way, speed key value pair
     */
    inline void
    add(const wayid_t &way, const bool &mph, const segment_speed_t &speed);

    /**
     * Checks if a way exists in the map
     */
    bool hasKey(const wayid_t &way) const;

    /**
     * Gets the value for a ways.
     * @throws a runtime_exception if the way is not found.
     */
    segment_speed_t getValue(const wayid_t &way) const;

    /**
     * Given a list of ways, returns the speed for each way.
     * If ways don't exist, the value in the result array will be INVALID_SPEED
     * There should be at least 1 values in in the `route` array
     */
    std::vector<segment_speed_t> getValues(const std::vector<wayid_t> &ways) const;

  private:
    sparse_hash_map<wayid_t, segment_speed_t> annotations;
};

#endif
