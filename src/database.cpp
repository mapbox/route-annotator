#include "database.hpp"

void Database::compact()
{
    rtree = std::make_unique<
        boost::geometry::index::rtree<value_t, boost::geometry::index::rstar<8>>>(
        used_nodes_list.begin(), used_nodes_list.end());

    // Tricks to free memory, swap out data with empty versions
    // This frees the memory.  shrink_to_fit doesn't guarantee that.
    std::vector<value_t>().swap(used_nodes_list);
    std::unordered_map<std::string, std::uint32_t>().swap(string_index);

    // Hint that these data structures can be shrunk.
    string_data.shrink_to_fit();
    string_offsets.shrink_to_fit();

    tagset_list.shrink_to_fit();
    key_value_pairs.shrink_to_fit();
    string_offsets.shrink_to_fit();
}

std::string Database::getstring(const std::size_t stringid)
{
    auto stringinfo = string_offsets[stringid];
    std::string result(string_data.begin() + stringinfo.first,
                       string_data.begin() + stringinfo.first + stringinfo.second);
    return result;
}

std::uint32_t Database::addstring(const char *str)
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
        string_offsets.emplace_back(
            static_cast<std::uint32_t>(string_data.size()) - string_length, string_length);
        idx = string_index.find(str);
    }
    BOOST_ASSERT(idx->second < std::numeric_limits<std::uint32_t>::max());
    return static_cast<std::uint32_t>(idx->second);
}

void Database::dump()
{
    std::cout << "String data is " << (string_data.capacity() * sizeof(char))
              << " Used: " << (string_data.size() * sizeof(char)) << "\n";
    std::cout << "tagset = Allocated "
              << (tagset_list.capacity() * sizeof(uint32_pairvector_t::value_type))
              << "  Used: " << (tagset_list.size() * sizeof(uint32_pairvector_t::value_type))
              << "\n";
    std::cout << "keyvalue = Allocated "
              << (key_value_pairs.capacity() * sizeof(uint32_pairvector_t::value_type))
              << "  Used: "
              << (key_value_pairs.size() * sizeof(uint32_pairvector_t::value_type)) << "\n";
    std::cout << "stringoffset = Allocated "
              << (string_offsets.capacity() * sizeof(uint32_pairvector_t::value_type))
              << "  Used: " << (string_offsets.size() * sizeof(uint32_pairvector_t::value_type))
              << "\n";

    std::cout << "pair_tagset_map = Allocated "
              << (pair_tagset_map.size() *
                  (sizeof(uint32_pairvector_t::value_type) + sizeof(tagsetid_t)))
              << "  Load factor: " << pair_tagset_map.load_factor()
              << " Buckets: " << pair_tagset_map.bucket_count() << "\n";
}
