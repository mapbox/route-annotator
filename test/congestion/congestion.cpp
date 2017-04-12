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
    std::ifstream file ("test/congestion/congestion.csv", std::ifstream::in);

    Hashmap hm(file);

    BOOST_CHECK_EQUAL(hm.getValue(62397298,62523814),20);
    BOOST_CHECK_EQUAL(hm.getValue(62444552,62444554),63);
    BOOST_CHECK_EQUAL(hm.getValue(62444554,62444556),63);
    BOOST_CHECK_EQUAL(hm.getValue(62444556,62444558),63);
    BOOST_CHECK_EQUAL(hm.getValue(62444554,62444552),64);

    BOOST_CHECK_EQUAL(hm.hasKey(62444554,62444552),true);
    BOOST_CHECK_EQUAL(hm.hasKey(100,100),false);
}
BOOST_AUTO_TEST_SUITE_END()