#include "segment_speed_map.hpp"
#include <sparsepp/spp.h>

#include <boost/fusion/adapted/std_pair.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/support_line_pos_iterator.hpp>

using spp::sparse_hash_map;

SegmentSpeedMap::SegmentSpeedMap(){};

SegmentSpeedMap::SegmentSpeedMap(const std::string &input_filename) { loadCSV(input_filename); }

SegmentSpeedMap::SegmentSpeedMap(const std::vector<std::string> &input_filenames)
{
    for (const auto &input_filename : input_filenames)
    {
        loadCSV(input_filename);
    }
}

void SegmentSpeedMap::loadCSV(const std::string &input_filename)
{
    namespace ph = boost::phoenix;
    namespace qi = boost::spirit::qi;

    boost::iostreams::mapped_file_source mmap(input_filename);
    auto first = mmap.begin(), last = mmap.end();
    qi::parse(first, last,
              -((qi::ulong_long >> ',' >> qi::ulong_long >> ',' >>
                 qi::uint_)[ph::bind(&SegmentSpeedMap::add, this, qi::_1, qi::_2, qi::_3)] %
                qi::eol) >>
                  *qi::eol);

    if (first != last)
    {
        auto bol = first - 1;
        while (bol > mmap.begin() && *bol != '\n')
            --bol;
        auto line_number = std::count(mmap.begin(), first, '\n') + 1;
        throw std::runtime_error("CSV parsing failed at " + input_filename + ':' +
                                 std::to_string(line_number) + ": " +
                                 std::string(bol + 1, std::find(first, last, '\n')));
    }
}

void SegmentSpeedMap::add(const external_nodeid_t &from,
                          const external_nodeid_t &to,
                          const segment_speed_t &speed)
{
    annotations[Segment(from, to)] = speed;
}

bool SegmentSpeedMap::hasKey(const external_nodeid_t &from, const external_nodeid_t &to) const
{
    return annotations.count(Segment(from, to)) > 0;
}

segment_speed_t SegmentSpeedMap::getValue(const external_nodeid_t &from,
                                          const external_nodeid_t &to) const
{
    // Save the result of find so that we don't need to repeat the lookup to get the value
    auto result = annotations.find(Segment(from, to));
    if (result == annotations.end())
    {
        throw std::runtime_error("Segment from NodeID " + std::to_string(from) + " to NodeId " +
                                 std::to_string(to) + " doesn't exist in the hashmap.");
    }

    // Use the already retrieved value as the result
    return result->second;
}

std::vector<segment_speed_t>
SegmentSpeedMap::getValues(const std::vector<external_nodeid_t> &route) const
{
    std::vector<segment_speed_t> speeds;
    if (route.size() < 2)
    {
        throw std::runtime_error(
            "NodeID Array should have more than 2 NodeIds for getValues method.");
    }

    speeds.resize(route.size() - 1);
    for (std::size_t segment_index = 0; segment_index < speeds.size(); ++segment_index)
    {
        auto from = route[segment_index];
        auto to = route[segment_index + 1];
        auto result = annotations.find(Segment(from, to));
        if (result == annotations.end())
        {
            speeds[segment_index] = INVALID_SPEED;
        }
        else
        {
            speeds[segment_index] = result->second;
        }
    }
    return speeds;
}
