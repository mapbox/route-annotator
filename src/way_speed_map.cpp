#include "way_speed_map.hpp"
#include <cmath>
#include <sparsepp/spp.h>

#include <boost/fusion/adapted/std_pair.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/support_line_pos_iterator.hpp>

using spp::sparse_hash_map;

WaySpeedMap::WaySpeedMap(){};

WaySpeedMap::WaySpeedMap(const std::string &input_filename) { loadCSV(input_filename); }

WaySpeedMap::WaySpeedMap(const std::vector<std::string> &input_filenames)
{
    for (const auto &input_filename : input_filenames)
    {
        loadCSV(input_filename);
    }
}

void WaySpeedMap::loadCSV(const std::string &input_filename)
{
    namespace ph = boost::phoenix;
    namespace qi = boost::spirit::qi;

    boost::iostreams::mapped_file_source mmap(input_filename);
    auto first = mmap.begin(), last = mmap.end();
    qi::parse(first, last,
              -((qi::uint_ >> ',' >> ((+(qi::char_ - ',')) | "" >> qi::lit("")) >> ',' >>
                 ("mph" >> qi::attr(true) | "kph" >> qi::attr(false) | "" >> qi::attr(false)) >>
                 ',' >> qi::uint_)[ph::bind(&WaySpeedMap::add, this, qi::_1, qi::_3, qi::_4)] %
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

void WaySpeedMap::add(const wayid_t &way, const bool &mph, const std::uint32_t &speed)
{

    if (mph)
    {
        std::uint32_t s = std::round(speed * 1.609);

        if (s > INVALID_SPEED - 1)
        {
            std::cout << "CSV parsing failed.  Way: " << std::to_string(way)
                      << " Speed: " << std::to_string(s) << std::endl;
        }
        else
            annotations[way] = s;
    }
    else
    {
        if (speed > INVALID_SPEED - 1)
        {
            std::cout << "CSV parsing failed.  From Node: " << std::to_string(way)
                      << " Speed: " << std::to_string(speed) << std::endl;
        }
        else
            annotations[way] = speed;
    }
}

bool WaySpeedMap::hasKey(const wayid_t &way) const { return (annotations.count(way) > 0); }

segment_speed_t WaySpeedMap::getValue(const wayid_t &way) const
{
    // Save the result of find so that we don't need to repeat the lookup to get the value
    auto result = annotations.find(way);
    if (result == annotations.end())
        throw std::runtime_error("Way ID " + std::to_string(way) +
                                 " doesn't exist in the hashmap.");

    // Use the already retrieved value as the result
    return result->second;
}

std::vector<segment_speed_t> WaySpeedMap::getValues(const std::vector<wayid_t> &route) const
{
    std::vector<segment_speed_t> speeds;
    if (route.size() < 1)
        throw std::runtime_error("Way Array should have at least 1 way ID for getValues method.");

    speeds.resize(route.size());
    for (std::size_t way_index = 0; way_index < speeds.size(); ++way_index)
    {
        auto result = annotations.find(route[way_index]);
        if (result == annotations.end())
            speeds[way_index] = INVALID_SPEED;
        else
            speeds[way_index] = result->second;
    }
    return speeds;
}
