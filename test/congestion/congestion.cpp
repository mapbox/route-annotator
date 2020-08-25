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
    BOOST_CHECK_EQUAL(map.getValue(297977101, 312122762), 5);
    BOOST_CHECK_EQUAL(map.getValue(69395079, 69402983), 23);
    BOOST_CHECK_EQUAL(map.getValue(3860306483, 1362215135), 6);
    BOOST_CHECK_EQUAL(map.getValue(1362215135, 297976455), 10);
}

BOOST_AUTO_TEST_CASE(congestion_test_get_values)
{
    SegmentSpeedMap map("test/congestion/fixtures/congestion.csv");

    std::vector<external_nodeid_t> nodes{86909055, 86909053,   86909050,   86909053,
                                         86909055, 3860306483, 1362215135, 297976455};
    std::vector<segment_speed_t> speeds{81, 81, 81, 81, INVALID_SPEED, 6, 10};
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

    nodes = {86909055, 86909053, 86909050, 86909053, 86909055, 3860306483, 1362215135, 297976455};
    expected = {81, 81, 81, 81, INVALID_SPEED, 6, 10};
    actual = map.getValues(nodes);
    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());

    nodes = {90, 91, 92, 93, 94, 297976455};
    expected = {2, 3, 4, 5, 10};
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

    nodes = {86909055, 86909053, 86909050, 86909053, 86909055, 3860306483, 1362215135, 297976455};
    expected = {81, 81, 81, 81, INVALID_SPEED, 6, 10};
    actual = map.getValues(nodes);
    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());

    nodes = {90, 91, 92, 93, 94, 297976455};
    expected = {INVALID_SPEED, INVALID_SPEED, INVALID_SPEED, INVALID_SPEED, INVALID_SPEED};
    actual = map.getValues(nodes);

    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());

    map.loadCSV("test/congestion/fixtures/congestion2.csv");

    expected = {2, 3, 4, 5, 10};
    actual = map.getValues(nodes);

    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(congestion_speeds_test_speed_greater_than_max)
{
    std::vector<std::string> paths = {"test/congestion/fixtures/congestion_speed_max.csv"};
    SegmentSpeedMap map(paths);

    std::vector<external_nodeid_t> nodes = {6909081, 86909079, 69402983};
    std::vector<segment_speed_t> expected = {159, INVALID_SPEED};
    std::vector<segment_speed_t> actual = map.getValues(nodes);
    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());

    map.loadCSV("test/congestion/fixtures/congestion_speed_max2.csv");
    expected = {159, INVALID_SPEED}; // first import was not overwritten.
    actual = map.getValues(nodes);
    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());
}

// Execptions
BOOST_AUTO_TEST_CASE(congestion_test_string_input)
{
    BOOST_CHECK_THROW(SegmentSpeedMap map("test/congestion/fixtures/string_input.csv"),
                      std::exception);
    BOOST_CHECK_THROW(SegmentSpeedMap map("test/congestion/fixtures/string_input2.csv"),
                      std::exception);
}

BOOST_AUTO_TEST_CASE(congestion_test_more_columns)
{
    BOOST_CHECK_THROW(SegmentSpeedMap map("test/congestion/fixtures/more_columns.csv"),
                      std::exception);
    BOOST_CHECK_THROW(SegmentSpeedMap map("test/congestion/fixtures/more_columns2.csv"),
                      std::exception);
}

BOOST_AUTO_TEST_CASE(congestion_test_fewer_columns)
{
    BOOST_CHECK_THROW(SegmentSpeedMap map("test/congestion/fixtures/fewer_columns.csv"),
                      std::exception);
    BOOST_CHECK_THROW(SegmentSpeedMap map("test/congestion/fixtures/fewer_columns2.csv"),
                      std::exception);
}

BOOST_AUTO_TEST_CASE(congestion_test_negative_number)
{
    BOOST_CHECK_THROW(SegmentSpeedMap map("test/congestion/fixtures/negative_number.csv"),
                      std::exception);
    BOOST_CHECK_THROW(SegmentSpeedMap map("test/congestion/fixtures/negative_number2.csv"),
                      std::exception);
}

BOOST_AUTO_TEST_CASE(congestion_test_invalid_csv)
{
    BOOST_CHECK_THROW(SegmentSpeedMap map("test/congestion/fixtures/invalid_csv.csv"),
                      std::exception);
    BOOST_CHECK_THROW(SegmentSpeedMap map("test/congestion/fixtures/invalid_csv2.csv"),
                      std::exception);
}

BOOST_AUTO_TEST_CASE(congestion_test_header)
{
    BOOST_CHECK_THROW(SegmentSpeedMap map("test/congestion/fixtures/header.csv"), std::exception);
    BOOST_CHECK_THROW(SegmentSpeedMap map("test/congestion/fixtures/header2.csv"), std::exception);
}

BOOST_AUTO_TEST_CASE(congestion_test_not_a_number)
{
    BOOST_CHECK_THROW(SegmentSpeedMap map("test/congestion/fixtures/not_a_number.csv"),
                      std::exception);
    BOOST_CHECK_THROW(SegmentSpeedMap map("test/congestion/fixtures/not_a_number2.csv"),
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
    BOOST_CHECK_THROW(map.getValue(207977101, 312122762), std::exception);
    BOOST_CHECK_THROW(map.getValue(63395079, 69402983), std::exception);
    BOOST_CHECK_THROW(map.getValue(3560306483, 1362215135), std::exception);
    BOOST_CHECK_THROW(map.getValue(1362215135, 297076455), std::exception);
}

BOOST_AUTO_TEST_CASE(congestion_test_get_values_exceptions)
{
    SegmentSpeedMap map("test/congestion/fixtures/congestion.csv");

    std::vector<external_nodeid_t> nodes{86909055};
    BOOST_CHECK_THROW(map.getValues(nodes), std::exception);

    nodes = {297076455};
    BOOST_CHECK_THROW(map.getValues(nodes), std::exception);

    nodes = {86909050, 86900053, 86900050, 297076455};
    std::vector<segment_speed_t> expected{INVALID_SPEED, INVALID_SPEED, INVALID_SPEED};
    std::vector<segment_speed_t> actual = map.getValues(nodes);
    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_SUITE_END()
