#pragma once
#include <string>
#include <utility>
#include <vector>

#include "database.hpp"
#include "types.hpp"

struct RouteAnnotator
{
  public:
    RouteAnnotator(const std::string &osmfilename);

    std::vector<internal_nodeid_t> coordinates_to_internal(const std::vector<point_t> &points);
    std::vector<internal_nodeid_t>
    external_to_internal(const std::vector<external_nodeid_t> &external_nodeids);

    annotated_route_t annotateRoute(const std::vector<internal_nodeid_t> &route);

    std::string get_tag_key(const std::size_t index);
    std::string get_tag_value(const std::size_t index);
    std::pair<std::uint32_t, std::uint32_t> get_tag_range(const std::size_t index);

  private:
    Database db;
};
