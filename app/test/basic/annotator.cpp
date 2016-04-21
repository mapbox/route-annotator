#include <boost/functional/hash.hpp>
#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test.hpp>

#include "annotator.hpp"
#include "database.hpp"

BOOST_AUTO_TEST_SUITE(annotator_test)

BOOST_AUTO_TEST_CASE(annotator_test_basic)
{

    Database db;
    const auto keyid = db.addstring("highway");
    const auto valueid = db.addstring("primary");
    db.key_value_pairs.emplace_back(keyid, valueid);
    db.way_tag_ranges.emplace_back(0,0);
    db.pair_way_map.emplace(internal_nodepair_t{0,1}, 0);
    db.compact();
    RouteAnnotator annotator(db);


    // Check for a route pair that exists
    std::vector<internal_nodeid_t> route{0,1};
    auto result = annotator.annotateRoute(route);
    BOOST_CHECK_EQUAL(result.size(), 1);
    BOOST_CHECK_EQUAL(result[0], 0);

    // Check for a route pair that doesn't exist
    route = std::vector<internal_nodeid_t>{1,2};
    result = annotator.annotateRoute(route);
    BOOST_CHECK_EQUAL(result.size(), 1);
    BOOST_CHECK_EQUAL(result[0], INVALID_WAYID);

    route = std::vector<internal_nodeid_t>{2,0,1,7};
    result = annotator.annotateRoute(route);
    BOOST_CHECK_EQUAL(result.size(), 3);
    BOOST_CHECK_EQUAL(result[0], INVALID_WAYID);
    BOOST_CHECK_EQUAL(result[1], 0);
    BOOST_CHECK_EQUAL(result[2], INVALID_WAYID);

    auto tagrange = annotator.get_tag_range(result[1]);
    BOOST_CHECK_EQUAL(tagrange.first, 0);
    BOOST_CHECK_EQUAL(tagrange.second, 0);

    BOOST_CHECK_EQUAL(annotator.get_tag_key(tagrange.first), "highway");
    BOOST_CHECK_EQUAL(annotator.get_tag_value(tagrange.first), "primary");

}


BOOST_AUTO_TEST_CASE(annotator_test_externalids)
{

    Database db;
    db.pair_way_map.emplace(internal_nodepair_t{0,1}, 0);
    db.external_internal_map.emplace(12345,7);
    db.external_internal_map.emplace(12346,9);
    db.external_internal_map.emplace(12347,13);
    db.compact();
    RouteAnnotator annotator(db);


    // Check that we match the expected coordinates
    std::vector<external_nodeid_t> external_nodeids{234857,12345,394875,12346};
    auto result = annotator.external_to_internal(external_nodeids);
    BOOST_CHECK_EQUAL(result.size(), 4);
    BOOST_CHECK_EQUAL(result[0], INVALID_INTERNAL_NODEID);
    BOOST_CHECK_EQUAL(result[1], 7);
    BOOST_CHECK_EQUAL(result[2], INVALID_INTERNAL_NODEID);
    BOOST_CHECK_EQUAL(result[3], 9);

}

BOOST_AUTO_TEST_CASE(annotator_test_coordinates)
{

    Database db;
    db.used_nodes_list.emplace_back(point_t{1, 1}, 7);
    db.used_nodes_list.emplace_back(point_t{2, 2}, 9);
    db.used_nodes_list.emplace_back(point_t{3, 3}, 13);
    db.compact();
    RouteAnnotator annotator(db);


    // Check that we match the expected coordinates
    std::vector<point_t> coordinates{point_t{1,1},point_t{1.5,1.5}, point_t{2,2}, point_t{3,1}};
    auto result = annotator.coordinates_to_internal(coordinates);
    BOOST_CHECK_EQUAL(result.size(), 4);
    BOOST_CHECK_EQUAL(result[0], 7);
    BOOST_CHECK_EQUAL(result[1], INVALID_INTERNAL_NODEID);
    BOOST_CHECK_EQUAL(result[2], 9);
    BOOST_CHECK_EQUAL(result[3], INVALID_INTERNAL_NODEID);

}


BOOST_AUTO_TEST_SUITE_END()
