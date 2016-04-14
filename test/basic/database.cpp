#include <boost/functional/hash.hpp>
#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test.hpp>

#include "annotator.hpp"
#include "database.hpp"

BOOST_AUTO_TEST_SUITE(database_test)

// Verify that the bearing-bounds checking function behaves as expected
BOOST_AUTO_TEST_CASE(database_string_test)
{
    BOOST_CHECK_EQUAL(true, true);

    Database db;

    BOOST_CHECK_THROW(db.getstring(0),std::runtime_error);
    // point_t a{1, 1};
    const auto id = db.addstring("test");
    db.compact();
    BOOST_CHECK_EQUAL(db.getstring(id),"test");
    BOOST_CHECK_EQUAL(id,0);

    BOOST_CHECK_THROW(db.getstring(id+1),std::runtime_error);
}

BOOST_AUTO_TEST_SUITE_END()
