#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test.hpp>

#include "hashmap.hpp"

BOOST_AUTO_TEST_SUITE(hashmap_test)

// Verify that the bearing-bounds checking function behaves as expected
BOOST_AUTO_TEST_CASE(hashmap_test_basic)
{
    Hashmap hm;

    hm.add(62397298, 62523814, 20);
    hm.add(62444552, 62444554, 63);
    hm.add(62444554, 62444556, 63);

    BOOST_CHECK_EQUAL(hm.getValue(62444552, 62444554), 63);
    BOOST_CHECK_EQUAL(hm.getValue(62397298, 62523814), 20);
    BOOST_CHECK_THROW(hm.getValue(62300298, 62523814), std::runtime_error);

    BOOST_CHECK_EQUAL(hm.hasKey(62397298, 62523814), true);
    BOOST_CHECK_EQUAL(hm.hasKey(60097298, 62523814), false);

    hm.add(1, 2, 1);
    hm.add(2, 3, 2);
    hm.add(3, 4, 3);

    std::vector<external_nodeid_t> ways_1 = {1, 2, 3, 4};
    std::vector<speed_t> expected_speeds_1 = {1, 2, 3};
    auto speeds_1 = hm.getValues(ways_1);
    for (std::size_t i = 0; i < speeds_1.size(); i++)
    {
        BOOST_CHECK_EQUAL(speeds_1[i], expected_speeds_1[i]);
    }

    std::vector<external_nodeid_t> ways_2 = {1, 2, 3, 4, 5};
    std::vector<speed_t> expected_speeds_2 = {1, 2, 3, INVALID_SPEED};
    auto speeds_2 = hm.getValues(ways_2);
    for (std::size_t i = 0; i < speeds_2.size(); i++)
    {
        BOOST_CHECK_EQUAL(speeds_2[i], expected_speeds_2[i]);
    }
}
BOOST_AUTO_TEST_SUITE_END()