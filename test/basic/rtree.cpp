#include <boost/functional/hash.hpp>
#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test.hpp>

#include "annotator.hpp"
#include "database.hpp"

BOOST_AUTO_TEST_SUITE(rtree_test)

// Verify that the bearing-bounds checking function behaves as expected
BOOST_AUTO_TEST_CASE(rtree_basic_test)
{
    Database db(true);

    db.used_nodes_list.emplace_back(point_t{1, 1}, 73);
    db.build_rtree();
    db.compact();
    BOOST_CHECK_EQUAL(db.rtree->size(), 1);
}

BOOST_AUTO_TEST_CASE(rtree_not_initialized)
{
    Database db;

    static const internal_nodeid_t TESTNODE{73};

    db.used_nodes_list.emplace_back(point_t{1, 1}, TESTNODE);

    RouteAnnotator annotator(db);

    std::vector<point_t> points{point_t{1, 1}};
    BOOST_CHECK_THROW(annotator.coordinates_to_internal(points), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(rtree_radius_test)
{
    Database db(true);

    static const internal_nodeid_t TESTNODE{73};

    db.used_nodes_list.emplace_back(point_t{1, 1}, TESTNODE);
    db.build_rtree();
    db.compact();

    RouteAnnotator annotator(db);

    std::vector<point_t> points{point_t{1, 1}};
    auto results = annotator.coordinates_to_internal(points);
    BOOST_CHECK_EQUAL(results.size(), 1);
    BOOST_CHECK_EQUAL(results[0], TESTNODE);

    // This should be within about 15km
    points = std::vector<point_t>{point_t{1.1, 1.1}};
    results = annotator.coordinates_to_internal(points);
    BOOST_CHECK_EQUAL(results.size(), 1);
    BOOST_CHECK_EQUAL(results[0], INVALID_INTERNAL_NODEID);

    // This should be within about 1.1m
    points = std::vector<point_t>{point_t{1.00005, 1}};
    results = annotator.coordinates_to_internal(points);
    BOOST_CHECK_EQUAL(results.size(), 1);
    BOOST_CHECK_EQUAL(results[0], INVALID_INTERNAL_NODEID);

    // This should be within about 10cm
    points = std::vector<point_t>{point_t{1.000001, 1}};
    results = annotator.coordinates_to_internal(points);
    BOOST_CHECK_EQUAL(results.size(), 1);
    BOOST_CHECK_EQUAL(results[0], TESTNODE);

    // Test at high latitudes
    db = Database(true);
    db.used_nodes_list.emplace_back(point_t{1, 65}, TESTNODE);
    db.build_rtree();
    db.compact();

    // This should be about 0.45m
    points = std::vector<point_t>{point_t{1.00001, 65}};
    results = annotator.coordinates_to_internal(points);
    BOOST_CHECK_EQUAL(results.size(), 1);
    BOOST_CHECK_EQUAL(results[0], TESTNODE);

    // This should be about 6.5m (so should NOT match)
    points = std::vector<point_t>{point_t{1.00013, 65}};
    results = annotator.coordinates_to_internal(points);
    BOOST_CHECK_EQUAL(results.size(), 1);
    BOOST_CHECK_EQUAL(results[0], INVALID_INTERNAL_NODEID);
}

BOOST_AUTO_TEST_SUITE_END()
