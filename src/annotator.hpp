#pragma once
#include <string>
#include <vector>
#include <utility>

#include "types.hpp"
#include "database.hpp"

struct AnnotatedRoute
{
    std::vector<int> tagset_offset_list;
    std::vector<tagsetid_t> tagset_list;
};

struct RouteAnnotator
{
    public:
        RouteAnnotator(const std::string &osmfilename);

        std::vector<internal_nodeid_t> coordinates_to_internal(const std::vector<point_t> & points);
        std::vector<internal_nodeid_t> external_to_internal(const std::vector<external_nodeid_t> & external_nodeids);

        AnnotatedRoute annotateRoute(const std::vector<internal_nodeid_t> & route);

        std::string get_tag_key(const std::size_t index);
        std::string get_tag_value(const std::size_t index);
        std::pair<std::uint32_t, std::uint32_t> get_tag_range(const std::size_t index);

    private:
        Database db;
};
