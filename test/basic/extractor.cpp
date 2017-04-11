#include <boost/functional/hash.hpp>
#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test.hpp>

#include "annotator.hpp"
#include "database.hpp"
#include "extractor.hpp"

#include <cstdio>
#include <fstream>
#include <iostream>

BOOST_AUTO_TEST_SUITE(extractor_test)

BOOST_AUTO_TEST_CASE(annotator_test_basic)
{

    std::string buffer("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                       "<osm generator=\"test\" version=\"0.6\">\n"
                       "<node id=\"101\" lon=\"1.0\" lat=\"1.0\"/>\n"
                       "<node id=\"202\" lon=\"1.0\" lat=\"2.0\"/>\n"
                       "<node id=\"303\" lon=\"1.0\" lat=\"3.0\"/>\n"
                       "<way id=\"99\">\n"
                       "  <nd ref=\"101\"/>\n"
                       "  <nd ref=\"202\"/>\n"
                       "  <nd ref=\"303\"/>\n"
                       "  <tag k=\"highway\" v=\"primary\"/>\n"
                       "</way>\n"
                       "</osm>");

    Database db;
    Extractor extractor(buffer.c_str(), buffer.size(), "xml", db);

    BOOST_CHECK_EQUAL(db.pair_way_map.size(), 4);
    BOOST_CHECK_EQUAL(db.external_internal_map.size(), 3);
    BOOST_CHECK_EQUAL(db.external_internal_map[101], 0);
    BOOST_CHECK_EQUAL(db.external_internal_map[202], 1);

    // TODO: check this - how should we handle items that don't exist ?
    BOOST_CHECK_EQUAL(db.external_internal_map[1], 0);
}

BOOST_AUTO_TEST_SUITE_END()
