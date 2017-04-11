#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test.hpp>
#include <fstream>
#include <iostream>

#include "segment_speed_map.hpp"
#include "types.hpp"

BOOST_AUTO_TEST_SUITE(congestions_test)

// Verify that the bearing-bounds checking function behaves as expected
BOOST_AUTO_TEST_CASE(congestion_test_basic)
{
    SegmentSpeedMap map("test/congestion/fixtures/congestion.csv");

    BOOST_CHECK_EQUAL(map.getValue(86909055, 86909053), 81);
    BOOST_CHECK_EQUAL(map.hasKey(100, 100), false);
}

BOOST_AUTO_TEST_CASE(congestion_test_many)
{
    SegmentSpeedMap map("test/congestion/fixtures/congestion.csv");

    BOOST_CHECK_EQUAL(map.getValue(86909055, 86909053), 81);
    BOOST_CHECK_EQUAL(map.getValue(86909053, 86909050), 81);
    BOOST_CHECK_EQUAL(map.getValue(86909050, 86909053), 81);
    BOOST_CHECK_EQUAL(map.getValue(86909053, 86909055), 81);
    BOOST_CHECK_EQUAL(map.getValue(86623322, 86622998), 89);
    BOOST_CHECK_EQUAL(map.getValue(86622998, 86623322), 88);
    BOOST_CHECK_EQUAL(map.getValue(86909074, 86909072), 79);
}

BOOST_AUTO_TEST_CASE(congestion_test_get_values)
{
    SegmentSpeedMap map("test/congestion/fixtures/congestion.csv");

    std::vector<external_nodeid_t> nodes{86909055, 86909053, 86909050, 86909053, 86909055};
    std::vector<segment_speed_t> speeds{81, 81, 81, 81};
    std::vector<segment_speed_t> response = map.getValues(nodes);
    BOOST_CHECK_EQUAL_COLLECTIONS(response.begin(), response.end(), speeds.begin(), speeds.end());
}

BOOST_AUTO_TEST_CASE(congestion_load_multiple)
{
    std::vector<external_nodeid_t> nodes;
    std::vector<segment_speed_t> expected;
    std::vector<segment_speed_t> actual;

    std::vector<std::string> paths = {"test/congestion/fixtures/congestion.csv",
                                      "test/congestion/fixtures/congestion2.csv"};
    SegmentSpeedMap map(paths);

    nodes = {86909055, 86909053, 86909050, 86909053, 86909055};
    expected = {81, 81, 81, 81};
    actual = map.getValues(nodes);
    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());

    nodes = {90, 91, 92, 93, 94};
    expected = {2, 3, 4, 5};
    actual = map.getValues(nodes);
    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(congestion_load_incremental)
{
    std::vector<external_nodeid_t> nodes;
    std::vector<segment_speed_t> expected;
    std::vector<segment_speed_t> actual;

    std::vector<std::string> paths = {"test/congestion/fixtures/congestion.csv"};
    SegmentSpeedMap map(paths);

    nodes = {86909055, 86909053, 86909050, 86909053, 86909055};
    expected = {81, 81, 81, 81};
    actual = map.getValues(nodes);
    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());

    nodes = {90, 91, 92, 93, 94};
    expected = {INVALID_SPEED, INVALID_SPEED, INVALID_SPEED, INVALID_SPEED};
    actual = map.getValues(nodes);

    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());

    map.loadCSV("test/congestion/fixtures/congestion2.csv");

    expected = {2, 3, 4, 5};
    actual = map.getValues(nodes);

    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());
}

// Execptions
BOOST_AUTO_TEST_CASE(congestion_test_string_input)
{
    BOOST_CHECK_THROW(SegmentSpeedMap map("test/congestion/fixtures/string_input.csv"),
                      std::exception);
}

BOOST_AUTO_TEST_CASE(congestion_test_more_columns)
{
    BOOST_CHECK_THROW(SegmentSpeedMap map("test/congestion/fixtures/more_columns.csv"),
                      std::exception);
}

BOOST_AUTO_TEST_CASE(congestion_test_fewer_columns)
{
    BOOST_CHECK_THROW(SegmentSpeedMap map("test/congestion/fixtures/fewer_columns.csv"),
                      std::exception);
}

BOOST_AUTO_TEST_CASE(congestion_test_negative_number)
{
    BOOST_CHECK_THROW(SegmentSpeedMap map("test/congestion/fixtures/negative_number.csv"),
                      std::exception);
}

BOOST_AUTO_TEST_CASE(congestion_test_invalid_csv)
{
    BOOST_CHECK_THROW(SegmentSpeedMap map("test/congestion/fixtures/invalid_csv.csv"),
                      std::exception);
}

BOOST_AUTO_TEST_CASE(congestion_test_header)
{
    BOOST_CHECK_THROW(SegmentSpeedMap map("test/congestion/fixtures/header.csv"), std::exception);
}

BOOST_AUTO_TEST_CASE(congestion_test_not_a_number)
{
    BOOST_CHECK_THROW(SegmentSpeedMap map("test/congestion/fixtures/not_a_number.csv"),
                      std::exception);
}

BOOST_AUTO_TEST_CASE(congestion_test_many_exceptions)
{
    SegmentSpeedMap map("test/congestion/fixtures/congestion.csv");

    BOOST_CHECK_THROW(map.getValue(86909050, 86900053), std::exception);
    BOOST_CHECK_THROW(map.getValue(86900053, 86900050), std::exception);
    BOOST_CHECK_THROW(map.getValue(86903150, 86900053), std::exception);
    BOOST_CHECK_THROW(map.getValue(86909253, 86900055), std::exception);
    BOOST_CHECK_THROW(map.getValue(86623122, 86620998), std::exception);
    BOOST_CHECK_THROW(map.getValue(86600998, 86620322), std::exception);
    BOOST_CHECK_THROW(map.getValue(86900074, 86900072), std::exception);
}

BOOST_AUTO_TEST_CASE(congestion_test_get_values_exceptions)
{
    SegmentSpeedMap map("test/congestion/fixtures/congestion.csv");

    std::vector<external_nodeid_t> nodes{86909055};
    BOOST_CHECK_THROW(map.getValues(nodes), std::exception);

    nodes = {86909050, 86900053, 86900050};
    std::vector<segment_speed_t> expected{INVALID_SPEED, INVALID_SPEED};
    std::vector<segment_speed_t> actual = map.getValues(nodes);
    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_SUITE_END()