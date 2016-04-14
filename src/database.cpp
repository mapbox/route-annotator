#include "database.hpp"

void Database::compact()
{
    rtree =
        std::make_unique<boost::geometry::index::rtree<value_t, boost::geometry::index::rstar<8>>>(
            used_nodes_list.begin(), used_nodes_list.end());

    // Tricks to free memory, swap out data with empty versions
    // This frees the memory.  shrink_to_fit doesn't guarantee that.
    std::vector<value_t>().swap(used_nodes_list);
    std::unordered_map<std::string, std::uint32_t>().swap(string_index);

    // Hint that these data structures can be shrunk.
    string_data.shrink_to_fit();
    string_offsets.shrink_to_fit();

    way_tag_ranges.shrink_to_fit();
    key_value_pairs.shrink_to_fit();
    string_offsets.shrink_to_fit();
}

std::string Database::getstring(const stringid_t stringid) const
{
    if (stringid >= string_offsets.size())
        throw std::runtime_error("Invalid string ID");

    auto stringinfo = string_offsets[stringid];
    std::string result(string_data.begin() + stringinfo.first,
                       string_data.begin() + stringinfo.first + stringinfo.second);
    return result;
}

stringid_t Database::addstring(const char *str)
{
    auto idx = string_index.find(str);
    if (idx == string_index.end())
    {
        BOOST_ASSERT(string_index.size() < std::numeric_limits<std::uint32_t>::max());
        string_index.emplace(str, static_cast<uint32_t>(string_index.size()));
        auto string_length =
            static_cast<std::uint32_t>(std::min<std::size_t>(255, std::strlen(str)));
        std::copy(str, str + string_length, std::back_inserter(string_data));
        BOOST_ASSERT(string_data.size() < std::numeric_limits<std::uint32_t>::max());
        BOOST_ASSERT(string_data.size() >= string_length);
        string_offsets.emplace_back(static_cast<std::uint32_t>(string_data.size()) - string_length,
                                    string_length);
        // Return the position we know we just added out string at
        return static_cast<std::uint32_t>(string_index.size() - 1);
    }
    BOOST_ASSERT(idx->second < std::numeric_limits<std::uint32_t>::max());
    return static_cast<std::uint32_t>(idx->second);
}

void Database::dump() const
{
    std::cout << "String data is " << (string_data.capacity() * sizeof(char))
              << " Used: " << (string_data.size() * sizeof(char)) << "\n";
    std::cout << "way_tag_ranges = Allocated "
              << (way_tag_ranges.capacity() * sizeof(decltype(way_tag_ranges)::value_type))
              << "  Used: "
              << (way_tag_ranges.size() * sizeof(decltype(way_tag_ranges)::value_type)) << "\n";
    std::cout << "keyvalue = Allocated "
              << (key_value_pairs.capacity() * sizeof(decltype(key_value_pairs)::value_type))
              << "  Used: "
              << (key_value_pairs.size() * sizeof(decltype(key_value_pairs)::value_type)) << "\n";
    std::cout << "stringoffset = Allocated "
              << (string_offsets.capacity() * sizeof(decltype(string_offsets)::value_type))
              << "  Used: "
              << (string_offsets.size() * sizeof(decltype(string_offsets)::value_type)) << "\n";

    std::cout << "pair_way_map = Allocated "
              << (pair_way_map.size() *
                  (sizeof(decltype(pair_way_map)::value_type) + sizeof(wayid_t)))
              << "  Load factor: " << pair_way_map.load_factor()
              << " Buckets: " << pair_way_map.bucket_count() << "\n";
}
