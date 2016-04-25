#pragma once

#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>

#include <sparsehash/sparse_hash_map>
//#include <stxxl/unordered_map>

// Type declarations for node ids and way ids.  We don't need the full 64 bits
// for ways as the highest way number isn't close to 2^32 yet.
typedef std::uint64_t external_nodeid_t;
typedef std::uint32_t internal_nodeid_t;
typedef std::uint32_t wayid_t;

static constexpr internal_nodeid_t INVALID_INTERNAL_NODEID =
    std::numeric_limits<internal_nodeid_t>::max();
static constexpr wayid_t INVALID_WAYID = std::numeric_limits<wayid_t>::max();

typedef boost::geometry::model::
    point<double, 2, boost::geometry::cs::spherical_equatorial<boost::geometry::degree>>
        point_t;
typedef std::pair<point_t, internal_nodeid_t> value_t;

// Data types for our lookup tables
typedef std::pair<internal_nodeid_t, internal_nodeid_t> internal_nodepair_t;

typedef google::sparse_hash_map<external_nodeid_t, internal_nodeid_t> external_internal_map_t;

typedef std::vector<wayid_t> annotated_route_t;

// Every unique string gets an ID of this type
typedef std::uint32_t stringid_t;

// A way has several tags.  The tagrange is the index of the
// first tag table entry, second is the index of the last
// tag table entry
typedef std::pair<std::uint32_t, std::uint32_t> tagrange_t;

// Pairs of string ids in the string table.  first = string ID
// of the key, second = string ID of the value
typedef std::pair<stringid_t, stringid_t> keyvalue_index_t;

// Indexes of strings in the char buffer.  first = first character
// position, second = length of string
typedef std::pair<std::uint32_t, std::uint32_t> stringoffset_t;

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
namespace tr1 {
template <> struct hash<internal_nodepair_t>
{
    inline size_t operator()(const internal_nodepair_t &v) const
    {
        return (static_cast<std::uint64_t>(v.first) << 32) + v.second;
    }
};
}
}
