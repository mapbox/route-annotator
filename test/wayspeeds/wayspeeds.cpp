#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test.hpp>
#include <fstream>
#include <iostream>

#include "way_speed_map.hpp"
#include "types.hpp"

BOOST_AUTO_TEST_SUITE(way_speeds_test)

// Verify that the bearing-bounds checking function behaves as expected
BOOST_AUTO_TEST_CASE(way_speeds_test_basic)
{
    WaySpeedMap map("test/wayspeeds/fixtures/way_speeds.csv");

    BOOST_CHECK_EQUAL(map.getValue(106817824), 113);
    BOOST_CHECK_EQUAL(map.hasKey(100), false);
}

BOOST_AUTO_TEST_CASE(way_speeds_test_many)
{
    WaySpeedMap map("test/wayspeeds/fixtures/way_speeds.csv");

    BOOST_CHECK_EQUAL(map.getValue(106817824), 113);
    BOOST_CHECK_EQUAL(map.getValue(231738435), 64);
    BOOST_CHECK_EQUAL(map.getValue(406215748), 48);
    BOOST_CHECK_EQUAL(map.getValue(301595694), 30);
    BOOST_CHECK_EQUAL(map.getValue(165499294), 70);
    BOOST_CHECK_EQUAL(map.getValue(49800696), 88);
}

BOOST_AUTO_TEST_CASE(way_speeds_test_get_values)
{
    WaySpeedMap map("test/wayspeeds/fixtures/way_speeds.csv");

    std::vector<wayid_t> ways{106817824, 231738435, 406215748, 301595694, 165499294, 49800696};
    std::vector<segment_speed_t> speeds{113, 64, 48, 30, 70, 88};
    std::vector<segment_speed_t> response = map.getValues(ways);
    BOOST_CHECK_EQUAL_COLLECTIONS(response.begin(), response.end(), speeds.begin(), speeds.end());
}

BOOST_AUTO_TEST_CASE(way_speeds_load_multiple)
{
    std::vector<wayid_t> ways;
    std::vector<segment_speed_t> expected;
    std::vector<segment_speed_t> actual;

    std::vector<std::string> paths = {"test/wayspeeds/fixtures/way_speeds.csv",
                                      "test/wayspeeds/fixtures/way_speeds2.csv"};
    WaySpeedMap map(paths);

    ways = {106817824, 231738435, 173681583, 45619838, 51369345, 171537086};
    expected = {113, 64, 19, 65, 129, 97};
    actual = map.getValues(ways);
    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(way_speeds_load_incremental)
{
    std::vector<wayid_t> ways;
    std::vector<segment_speed_t> expected;
    std::vector<segment_speed_t> actual;

    std::vector<std::string> paths = {"test/wayspeeds/fixtures/way_speeds.csv"};
    WaySpeedMap map(paths);

    ways = {106817824, 231738435, 173681583, 45619838, 51369345, 171537086};
    expected = {113, 64, INVALID_SPEED, INVALID_SPEED, INVALID_SPEED, 113};
    actual = map.getValues(ways);
    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());

    map.loadCSV("test/wayspeeds/fixtures/way_speeds2.csv");
    expected = {113, 64, 19, 65, 129, 97};
    actual = map.getValues(ways);

    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());
}

// Execptions
BOOST_AUTO_TEST_CASE(way_speeds_test_string_input)
{
    BOOST_CHECK_THROW(WaySpeedMap map("test/wayspeeds/fixtures/string_input.csv"),
                      std::exception);
}

BOOST_AUTO_TEST_CASE(way_speeds_test_more_columns)
{
    BOOST_CHECK_THROW(WaySpeedMap map("test/wayspeeds/fixtures/more_columns.csv"),
                      std::exception);
}

BOOST_AUTO_TEST_CASE(way_speeds_test_fewer_columns)
{
    BOOST_CHECK_THROW(WaySpeedMap map("test/wayspeeds/fixtures/fewer_columns.csv"),
                      std::exception);
}

BOOST_AUTO_TEST_CASE(way_speeds_test_negative_number)
{
    BOOST_CHECK_THROW(WaySpeedMap map("test/wayspeeds/fixtures/negative_number.csv"),
                      std::exception);
}

BOOST_AUTO_TEST_CASE(way_speeds_test_invalid_csv)
{
    BOOST_CHECK_THROW(WaySpeedMap map("test/wayspeeds/fixtures/invalid_csv.csv"),
                      std::exception);
}

BOOST_AUTO_TEST_CASE(way_speeds_test_header)
{
    BOOST_CHECK_THROW(WaySpeedMap map("test/wayspeeds/fixtures/header.csv"), std::exception);
}

BOOST_AUTO_TEST_CASE(way_speeds_test_not_a_number)
{
    BOOST_CHECK_THROW(WaySpeedMap map("test/wayspeeds/fixtures/not_a_number.csv"),
                      std::exception);
}

BOOST_AUTO_TEST_CASE(way_speeds_test_speed_greater_than_max)
{
    BOOST_CHECK_THROW(WaySpeedMap map("test/wayspeeds/fixtures/way_speed_max.csv"),
                      std::exception);
}

BOOST_AUTO_TEST_CASE(way_speeds_test_many_exceptions)
{
    WaySpeedMap map("test/wayspeeds/fixtures/way_speeds.csv");

    BOOST_CHECK_THROW(map.getValue(1), std::exception);
    BOOST_CHECK_THROW(map.getValue(3), std::exception);
    BOOST_CHECK_THROW(map.getValue(6988117), std::exception);
}

BOOST_AUTO_TEST_CASE(way_speeds_test_get_values_exceptions)
{
    WaySpeedMap map("test/wayspeeds/fixtures/way_speeds.csv");

    std::vector<wayid_t> ways{};
    BOOST_CHECK_THROW(map.getValues(ways), std::exception);

    ways = {1, 2, 3};
    std::vector<segment_speed_t> expected{INVALID_SPEED, INVALID_SPEED, INVALID_SPEED};
    std::vector<segment_speed_t> actual = map.getValues(ways);
    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(way_speeds_test_has_key)
{
    WaySpeedMap map("test/wayspeeds/fixtures/way_speeds.csv");

    BOOST_CHECK_EQUAL(map.hasKey(106817824), true);
    BOOST_CHECK_EQUAL(map.hasKey(1), false);

    BOOST_CHECK_EQUAL(map.hasKey(51369345),false);
    map.loadCSV("test/wayspeeds/fixtures/way_speeds2.csv");
    BOOST_CHECK_EQUAL(map.hasKey(51369345),true);
}

BOOST_AUTO_TEST_SUITE_END()
