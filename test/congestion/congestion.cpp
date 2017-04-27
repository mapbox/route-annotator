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
    Hashmap hm("test/congestion/congestion.csv");

	BOOST_CHECK_EQUAL(hm.getValue(86909053,86909055),81);
    BOOST_CHECK_EQUAL(hm.hasKey(100,100),false);
}

BOOST_AUTO_TEST_CASE(congestion_test_many)
{
    Hashmap hm("test/congestion/congestion.csv");

	BOOST_CHECK_EQUAL(hm.getValue(86909053,86909055),81);
	BOOST_CHECK_EQUAL(hm.getValue(86909050,86909053),81);
	BOOST_CHECK_EQUAL(hm.getValue(86909053,86909050),81);
	BOOST_CHECK_EQUAL(hm.getValue(86909055,86909053),81);
	BOOST_CHECK_EQUAL(hm.getValue(86622998,86623322),89);
	BOOST_CHECK_EQUAL(hm.getValue(86623322,86622998),88);
	BOOST_CHECK_EQUAL(hm.getValue(86909072,86909074),79);
}

BOOST_AUTO_TEST_CASE(congestion_test_fails)
{
    Hashmap hm("test/congestion/congestion.csv");

	BOOST_CHECK_THROW(hm.getValue(86909053,86902055), std::exception);
	BOOST_CHECK_THROW(hm.getValue(86909050,86901053), std::exception);
	BOOST_CHECK_THROW(hm.getValue(86909053,86903150), std::exception);
	BOOST_CHECK_THROW(hm.getValue(86909055,86909253), std::exception);
	BOOST_CHECK_THROW(hm.getValue(86622998,86623122), std::exception);
	BOOST_CHECK_THROW(hm.getValue(86623322,86600998), std::exception);
	BOOST_CHECK_THROW(hm.getValue(86909072,86900074), std::exception);
}

BOOST_AUTO_TEST_SUITE_END()