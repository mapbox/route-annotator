#pragma once

#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>

#include <unordered_map>

// Type declarations for node ids and way ids.  We don't need the full 64 bits
// for ways as the highest way number isn't close to 2^32 yet.
typedef std::uint64_t external_nodeid_t;
typedef std::uint32_t internal_nodeid_t;
typedef std::uint32_t tagsetid_t;

static constexpr internal_nodeid_t INVALID_INTERNAL_NODEID = std::numeric_limits<internal_nodeid_t>::max();

typedef boost::geometry::model::
    point<double, 2, boost::geometry::cs::spherical_equatorial<boost::geometry::degree>> point_t;
typedef std::pair<point_t, internal_nodeid_t> value_t;

// Data types for our lookup tables
typedef std::pair<internal_nodeid_t, internal_nodeid_t> internal_nodepair_t;

typedef std::unordered_map<external_nodeid_t, internal_nodeid_t> external_internal_map_t;

// Our tagmap is a set of indexes.  We don't store the strings themselves, we store
// indexes into our string lookup tables, which only stores each unique string once.
typedef std::vector<std::pair<std::uint32_t, std::uint32_t>> uint32_pairvector_t;

// Custom hashing functions for our data types to use in standard containers (sets, maps, etc).
namespace std
{
// This is so that we have reasonable hashing of pairs of nodeids
template <> struct hash<internal_nodepair_t>
{
    inline size_t operator()(const internal_nodepair_t &v) const
    {
        return (static_cast<std::uint64_t>(v.first) << 32) + v.second;
    }
};
}
