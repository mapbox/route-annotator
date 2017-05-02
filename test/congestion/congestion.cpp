#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test.hpp>
#include <fstream>
#include <iostream>

#include "hashmap.hpp"
#include "types.hpp"

BOOST_AUTO_TEST_SUITE(congestions_test)

// Verify that the bearing-bounds checking function behaves as expected
BOOST_AUTO_TEST_CASE(congestion_test_basic)
{
    Hashmap hm("test/congestion/fixtures/congestion.csv");

	BOOST_CHECK_EQUAL(hm.getValue(86909055,86909053),81);
    BOOST_CHECK_EQUAL(hm.hasKey(100,100),false);
}

BOOST_AUTO_TEST_CASE(congestion_test_many)
{
    Hashmap hm("test/congestion/fixtures/congestion.csv");

	BOOST_CHECK_EQUAL(hm.getValue(86909055,86909053),81);
	BOOST_CHECK_EQUAL(hm.getValue(86909053,86909050),81);
	BOOST_CHECK_EQUAL(hm.getValue(86909050,86909053),81);
	BOOST_CHECK_EQUAL(hm.getValue(86909053,86909055),81);
	BOOST_CHECK_EQUAL(hm.getValue(86623322,86622998),89);
	BOOST_CHECK_EQUAL(hm.getValue(86622998,86623322),88);
	BOOST_CHECK_EQUAL(hm.getValue(86909074,86909072),79);
}

BOOST_AUTO_TEST_CASE(congestion_test_get_values)
{
    Hashmap hm("test/congestion/fixtures/congestion.csv");

	std::vector<external_nodeid_t> nodes{86909055, 86909053, 86909050, 86909053, 86909055};
	std::vector<congestion_speed_t> speeds{81, 81, 81, 81};
	std::vector<congestion_speed_t> response = hm.getValues(nodes);
	BOOST_CHECK_EQUAL_COLLECTIONS(response.begin(), response.end(), speeds.begin(), speeds.end());
}

// Execptions
BOOST_AUTO_TEST_CASE(congestion_test_string_input)
{
    BOOST_CHECK_THROW(Hashmap hm("test/congestion/fixtures/string_input.csv"), std::exception);
}

BOOST_AUTO_TEST_CASE(congestion_test_more_columns)
{
    BOOST_CHECK_THROW(Hashmap hm("test/congestion/fixtures/more_columns.csv"), std::exception);
}

BOOST_AUTO_TEST_CASE(congestion_test_fewer_columns)
{
    BOOST_CHECK_THROW(Hashmap hm("test/congestion/fixtures/fewer_columns.csv"), std::exception);
}

BOOST_AUTO_TEST_CASE(congestion_test_negative_number)
{
    BOOST_CHECK_THROW(Hashmap hm("test/congestion/fixtures/negative_number.csv"), std::exception);
}

BOOST_AUTO_TEST_CASE(congestion_test_invalid_csv)
{
    BOOST_CHECK_THROW(Hashmap hm("test/congestion/fixtures/invalid_csv.csv"), std::exception);
}

BOOST_AUTO_TEST_CASE(congestion_test_header)
{
    BOOST_CHECK_THROW(Hashmap hm("test/congestion/fixtures/header.csv"), std::exception);
}

BOOST_AUTO_TEST_CASE(congestion_test_not_a_number)
{
    BOOST_CHECK_THROW(Hashmap hm("test/congestion/fixtures/not_a_number.csv"), std::exception);
}

BOOST_AUTO_TEST_CASE(congestion_test_many_exceptions)
{
    Hashmap hm("test/congestion/fixtures/congestion.csv");

	BOOST_CHECK_THROW(hm.getValue(86909050,86900053), std::exception);
	BOOST_CHECK_THROW(hm.getValue(86900053,86900050), std::exception);
	BOOST_CHECK_THROW(hm.getValue(86903150,86900053), std::exception);
	BOOST_CHECK_THROW(hm.getValue(86909253,86900055), std::exception);
	BOOST_CHECK_THROW(hm.getValue(86623122,86620998), std::exception);
	BOOST_CHECK_THROW(hm.getValue(86600998,86620322), std::exception);
	BOOST_CHECK_THROW(hm.getValue(86900074,86900072), std::exception);
}

BOOST_AUTO_TEST_CASE(congestion_test_get_values_exceptions)
{
    Hashmap hm("test/congestion/fixtures/congestion.csv");

	std::vector<external_nodeid_t> nodes{86909055};
	BOOST_CHECK_THROW(hm.getValues(nodes), std::exception);

	std::vector<external_nodeid_t> nodes2{86909050, 86900053, 86900050};
	std::vector<congestion_speed_t> speeds{INVALID_SPEED, INVALID_SPEED};
	std::vector<congestion_speed_t> response = hm.getValues(nodes2);
	BOOST_CHECK_EQUAL_COLLECTIONS(response.begin(), response.end(), speeds.begin(), speeds.end());
}

BOOST_AUTO_TEST_SUITE_END()