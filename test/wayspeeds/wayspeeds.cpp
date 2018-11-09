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

    BOOST_CHECK_EQUAL(map.getValue(286508206), 5);
    BOOST_CHECK_EQUAL(map.hasKey(100), false);
}

BOOST_AUTO_TEST_CASE(way_speeds_test_many)
{
    WaySpeedMap map("test/wayspeeds/fixtures/way_speeds.csv");

    BOOST_CHECK_EQUAL(map.getValue(11750872), 50);
    BOOST_CHECK_EQUAL(map.getValue(286508200), 5);
    BOOST_CHECK_EQUAL(map.getValue(11753880), 40);
    BOOST_CHECK_EQUAL(map.getValue(15591960), 30);
    BOOST_CHECK_EQUAL(map.getValue(563386869), 64);
    BOOST_CHECK_EQUAL(map.getValue(11736165), 50);
}

BOOST_AUTO_TEST_CASE(way_speeds_test_get_values)
{
    WaySpeedMap map("test/wayspeeds/fixtures/way_speeds.csv");

    std::vector<wayid_t> ways{11750872, 286508200, 11753880, 15591960, 563386869, 11736165};
    std::vector<segment_speed_t> speeds{50, 5, 40, 30, 64, 50};
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

    ways = {6697274, 97518240, 6692620, 97518240, 6688117, 11714049};
    expected = {60, 31, 30, 31, 72, 32};
    actual = map.getValues(ways);
    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());

    ways = {11750872, 286508200, 11753880, 15591960, 563386869, 11736165};
    expected = {50, 5, 40, 30, 64, 50};
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

    ways = {11750872, 286508200, 11753880, 15591960, 563386869, 11736165};
    expected = {50, 5, 40, 30, 64, 50};
    actual = map.getValues(ways);
    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());

    ways = {6697274, 97518240, 6692620, 97518240, 11714049, 6688117};
    expected = {INVALID_SPEED, INVALID_SPEED, INVALID_SPEED, INVALID_SPEED, 30, INVALID_SPEED};
    actual = map.getValues(ways);

    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());

    map.loadCSV("test/wayspeeds/fixtures/way_speeds2.csv");
    expected = {60, 31, 30, 31, 32, 72};
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

    BOOST_CHECK_EQUAL(map.hasKey(11736165), true);
    BOOST_CHECK_EQUAL(map.hasKey(1), false);

    BOOST_CHECK_EQUAL(map.hasKey(6697274),false);
    map.loadCSV("test/wayspeeds/fixtures/way_speeds2.csv");
    BOOST_CHECK_EQUAL(map.hasKey(6697274),true);
}

BOOST_AUTO_TEST_SUITE_END()
