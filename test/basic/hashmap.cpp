#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test.hpp>

#include "segment_speed_map.hpp"

BOOST_AUTO_TEST_SUITE(segment_speed_map_test)

// Verify that the bearing-bounds checking function behaves as expected
BOOST_AUTO_TEST_CASE(segment_speed_map_test_basic)
{
    SegmentSpeedMap map;

    map.add(62397298, 62523814, 20);
    map.add(62444552, 62444554, 63);
    map.add(62444554, 62444556, 63);

    BOOST_CHECK_EQUAL(map.getValue(62444552, 62444554), 63);
    BOOST_CHECK_EQUAL(map.getValue(62397298, 62523814), 20);
    BOOST_CHECK_THROW(map.getValue(62300298, 62523814), std::runtime_error);

    BOOST_CHECK_EQUAL(map.hasKey(62397298, 62523814), true);
    BOOST_CHECK_EQUAL(map.hasKey(60097298, 62523814), false);

    map.add(1, 2, 1);
    map.add(2, 3, 2);
    map.add(3, 4, 3);

    std::vector<external_nodeid_t> ways_1 = {1, 2, 3, 4};
    std::vector<speed_t> expected_speeds_1 = {1, 2, 3};
    auto speeds_1 = map.getValues(ways_1);
    for (std::size_t i = 0; i < speeds_1.size(); i++)
    {
        BOOST_CHECK_EQUAL(speeds_1[i], expected_speeds_1[i]);
    }

    std::vector<external_nodeid_t> ways_2 = {1, 2, 3, 4, 5};
    std::vector<speed_t> expected_speeds_2 = {1, 2, 3, INVALID_SPEED};
    auto speeds_2 = map.getValues(ways_2);
    for (std::size_t i = 0; i < speeds_2.size(); i++)
    {
        BOOST_CHECK_EQUAL(speeds_2[i], expected_speeds_2[i]);
    }
}
BOOST_AUTO_TEST_SUITE_END()