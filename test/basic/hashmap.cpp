#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test.hpp>

#include "hashmap.hpp"

BOOST_AUTO_TEST_SUITE(hashmap_test)

// Verify that the bearing-bounds checking function behaves as expected
BOOST_AUTO_TEST_CASE(hashmap_test_basic)
{
    Hashmap hm;

    hm.add(62397298,62523814,20);
    hm.add(62444552,62444554,63);
    hm.add(62444554,62444556,63);

    BOOST_CHECK_EQUAL(hm.getValue(62444552,62444554), 63);
    BOOST_CHECK_EQUAL(hm.getValue(62397298,62523814), 20);

    BOOST_CHECK_EQUAL(hm.hasKey(62397298,62523814), true);
    BOOST_CHECK_EQUAL(hm.hasKey(60097298,62523814), false);
}
BOOST_AUTO_TEST_SUITE_END()