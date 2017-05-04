#include "hashmap.hpp"
#include "csv.hpp"
#include <sparsepp/spp.h>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/support_line_pos_iterator.hpp>
#include <boost/fusion/adapted/std_pair.hpp>

Hashmap::Hashmap(){};

#if 0

using spp::sparse_hash_map;

Hashmap::Hashmap(const std::string &input_filename)
{

    io::CSVReader<3> in(input_filename);
    in.set_header("from", "to", "speed");
    external_nodeid_t from;
    external_nodeid_t to;
    congestion_speed_t speed;
    // Pre-allocate a large chunk of memory to save on all the micro-allocations
    // that would happen if adding items one-by-one and growing as needed
    // This is the size of the wholeworld.csv file that contains all of the freeflow data currently
    // (as of May 2, 2017)
    annotations.reserve(139064548);
    while (in.read_row(from, to, speed))
    {
        add(from, to, speed);
    }
}

#else

struct Segment final
{
    std::uint64_t from, to;
    Segment() : from(0), to(0) {}
    Segment(const std::uint64_t from, const std::uint64_t to) : from(from), to(to) {}

    bool operator<(const Segment &rhs) const
    {
        return std::tie(from, to) < std::tie(rhs.from, rhs.to);
    }
    bool operator==(const Segment &rhs) const
    {
        return std::tie(from, to) == std::tie(rhs.from, rhs.to);
    }
};

struct SpeedSource final
{
    SpeedSource() : speed(0), rate(std::numeric_limits<double>::quiet_NaN()) {}
    unsigned speed;
    double rate;
};

BOOST_FUSION_ADAPT_STRUCT(Segment,
                         (decltype(Segment::from), from)
                         (decltype(Segment::to), to))
BOOST_FUSION_ADAPT_STRUCT(SpeedSource,
                          (decltype(SpeedSource::speed), speed)
                          (decltype(SpeedSource::rate), rate))

Hashmap::Hashmap(const std::string &input_filename) {

    // Pre-allocate a large chunk of memory to save on all the micro-allocations
    // that would happen if adding items one-by-one and growing as needed
    // This is the size of the wholeworld.csv file that contains all of the freeflow data currently
    // (as of May 2, 2017)
    annotations.reserve(139064548);

    using Iterator = boost::spirit::line_pos_iterator<boost::spirit::istream_iterator>;
    using KeyRule = boost::spirit::qi::rule<Iterator, Segment()>;
    using ValueRule = boost::spirit::qi::rule<Iterator, SpeedSource()>;

    KeyRule key_rule;
    key_rule = boost::spirit::qi::ulong_long >> ',' >> boost::spirit::qi::ulong_long;
    ValueRule value_rule;
    value_rule = boost::spirit::qi::uint_ >> -(',' >> boost::spirit::qi::double_);

    std::ifstream input_stream(input_filename, std::ios::binary);
    input_stream.unsetf(std::ios::skipws);

    boost::spirit::istream_iterator sfirst(input_stream), slast;
    Iterator first(sfirst), last(slast);

    BOOST_ASSERT(file_id <= std::numeric_limits<std::uint8_t>::max());
    boost::spirit::qi::rule<Iterator, std::pair<Segment, SpeedSource>()> csv_line =
        (key_rule >> ',' >> value_rule) >> -(',' >> *(boost::spirit::qi::char_ - boost::spirit::qi::eol));
    std::vector<std::pair<Segment, SpeedSource>> results;
    const auto ok = boost::spirit::qi::parse(first, last, -(csv_line % boost::spirit::qi::eol) >> *boost::spirit::qi::eol, results);

    for (auto &result : results) {
        add(result.first.from, result.first.to, result.second.speed);
    }
}
#endif

void Hashmap::add(const external_nodeid_t &from,
                  const external_nodeid_t &to,
                  const congestion_speed_t &speed)
{
    annotations[Way(from, to)] = speed;
};

bool Hashmap::hasKey(const external_nodeid_t &from, const external_nodeid_t &to) const
{
    return annotations.count(Way(from, to)) > 0;
};

congestion_speed_t Hashmap::getValue(const external_nodeid_t &from,
                                     const external_nodeid_t &to) const
{
    // Save the result of find so that we don't need to repeat the lookup to get the value
    auto result = annotations.find(Way(from, to));
    if (result == annotations.end())
    {
        throw std::runtime_error("Way from NodeID " + std::to_string(from) + " to NodeId " +
                                 std::to_string(to) + " doesn't exist in the hashmap.");
    }

    // Use the already retrieved value as the result
    return result->second;
};

std::vector<congestion_speed_t> Hashmap::getValues(const std::vector<external_nodeid_t> &way) const
{
    std::vector<congestion_speed_t> speeds;
    if (way.size() < 2)
    {
        throw std::runtime_error(
            "NodeID Array should have more than 2 NodeIds for getValues methodr.");
    }

    if (way.size() > 1)
    {
        speeds.resize(way.size() - 1);
        for (std::size_t node_id = 0; node_id < speeds.size(); node_id++)
        {
            if (hasKey(way[node_id], way[node_id + 1]))
            {
                speeds[node_id] = getValue(way[node_id], way[node_id + 1]);
            }
            else
            {
                speeds[node_id] = INVALID_SPEED;
            }
        }
    }
    return speeds;
};
