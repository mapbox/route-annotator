#include "hashmap.hpp"
#include <sparsepp/spp.h>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/support_line_pos_iterator.hpp>

using spp::sparse_hash_map;

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

using Iterator = boost::spirit::line_pos_iterator<boost::spirit::istream_iterator>;
using KeyRule = boost::spirit::qi::rule<Iterator, Segment()>;
using ValueRule = boost::spirit::qi::rule<Iterator, SpeedSource()>;

Hashmap::Hashmap() {};

Hashmap::Hashmap(const std::string &input_filename) {

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

    if (!ok || first != last)
    {
        // const auto message =
        //     boost::format("CSV file %1% malformed on line %2%") % filename % first.position();
        // throw util::exception(message.str() + SOURCE_REF);
    }

    // util::Log() << "Loaded " << filename << " with " << results.size() << "values";

    for (auto &result : results) {
        add(result.first.to, result.first.from, result.second.speed);
    }

};

// Hashmap::Hashmap(const std::string input_filename) {
//     std::ifstream input(input_filename, std::ifstream::in);

//     std::string line;
//     external_nodeid_t to;
//     external_nodeid_t from;
//     congestion_speed_t speed;
//     std::string str_to;
//     std::string str_from;
//     std::string str_speed;

//     if (input)
//     {
//         while (getline(input, line))
//         {
//             try
//             {
//                 std::stringstream iss;
//                 iss << line;
//                 std::getline(iss, str_to, ',');
//                 to = std::stoull(str_to);
//                 std::getline(iss, str_from, ',');
//                 from = std::stoull(str_from);
//                 std::getline(iss, str_speed, ',');
//                 speed = std::stoull(str_speed);

//                 add(to, from, speed);
//             }
//             catch (std::exception& e)
//             {
//                 std::cout << "Input input has invalid format." << std::endl;
//                 std::cout << e.what() << std::endl;
//             }
//         }
//     }
//     else {
//         BOOST_ASSERT_MSG(false, 'no inputfile');
//     }
//     input.close();
// };

void Hashmap::add(external_nodeid_t to, external_nodeid_t from, congestion_speed_t speed) {
    Hashmap::annotations[Way(to, from)] = speed;
};

bool Hashmap::hasKey(external_nodeid_t to, external_nodeid_t from) const {
    const auto lookup = annotations.find(Way(to,from));
    if(lookup != annotations.end()) {
        return true;
    } else {
        return false;
    }
};

// @TODO use hasKey to get pointer and directly return
congestion_speed_t Hashmap::getValue(external_nodeid_t to, external_nodeid_t from) const
{
    if (!Hashmap::hasKey(to, from)) {
        throw std::runtime_error("Way from NodeID " + std::to_string(to) + "to NodeId " + std::to_string(from) + " doesn't exist in the hashmap.");
    }

    return annotations.at(Way(to, from));
};

std::vector<congestion_speed_t> Hashmap::getValues(std::vector<external_nodeid_t>& way) const
{
    std::vector<congestion_speed_t> speeds;
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
