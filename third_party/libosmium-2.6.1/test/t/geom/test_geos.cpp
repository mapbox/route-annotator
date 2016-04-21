#include "catch.hpp"

#include <osmium/geom/geos.hpp>

#include "area_helper.hpp"
#include "wnl_helper.hpp"

TEST_CASE("GEOS geometry factory - create point") {
    osmium::geom::GEOSFactory<> factory;

    std::unique_ptr<geos::geom::Point> point {factory.create_point(osmium::Location(3.2, 4.2))};
    REQUIRE(3.2 == point->getX());
    REQUIRE(4.2 == point->getY());
    REQUIRE(-1 == point->getSRID());
}

TEST_CASE("GEOS geometry factory - create point with non-default srid") {
    osmium::geom::GEOSFactory<> factory(4326);

    std::unique_ptr<geos::geom::Point> point {factory.create_point(osmium::Location(3.2, 4.2))};
    REQUIRE(3.2 == point->getX());
    REQUIRE(4.2 == point->getY());
    REQUIRE(4326 == point->getSRID());
}

TEST_CASE("GEOS geometry factory - create point with externally created GEOS factory") {
    geos::geom::GeometryFactory geos_factory;
    osmium::geom::GEOSFactory<> factory(geos_factory);

    std::unique_ptr<geos::geom::Point> point {factory.create_point(osmium::Location(3.2, 4.2))};
    REQUIRE(3.2 == point->getX());
    REQUIRE(4.2 == point->getY());
    REQUIRE(0 == point->getSRID());
}

TEST_CASE("GEOS geometry factory - can not create from invalid location") {
    osmium::geom::GEOSFactory<> factory;

    REQUIRE_THROWS_AS(factory.create_point(osmium::Location()), osmium::invalid_location);
}

TEST_CASE("GEOS geometry factory - create linestring") {
    osmium::geom::GEOSFactory<> factory;

    osmium::memory::Buffer buffer(10000);
    auto &wnl = create_test_wnl_okay(buffer);

    SECTION("from way node list") {
        std::unique_ptr<geos::geom::LineString> linestring {factory.create_linestring(wnl)};
        REQUIRE(3 == linestring->getNumPoints());

        std::unique_ptr<geos::geom::Point> p0 = std::unique_ptr<geos::geom::Point>(linestring->getPointN(0));
        REQUIRE(3.2 == p0->getX());
        std::unique_ptr<geos::geom::Point> p2 = std::unique_ptr<geos::geom::Point>(linestring->getPointN(2));
        REQUIRE(3.6 == p2->getX());
    }

    SECTION("without duplicates and backwards") {
        std::unique_ptr<geos::geom::LineString> linestring {factory.create_linestring(wnl, osmium::geom::use_nodes::unique, osmium::geom::direction::backward)};
        REQUIRE(3 == linestring->getNumPoints());
        std::unique_ptr<geos::geom::Point> p0 = std::unique_ptr<geos::geom::Point>(linestring->getPointN(0));
        REQUIRE(3.6 == p0->getX());
        std::unique_ptr<geos::geom::Point> p2 = std::unique_ptr<geos::geom::Point>(linestring->getPointN(2));
        REQUIRE(3.2 == p2->getX());
    }

    SECTION("with duplicates") {
        std::unique_ptr<geos::geom::LineString> linestring {factory.create_linestring(wnl, osmium::geom::use_nodes::all)};
        REQUIRE(4 == linestring->getNumPoints());
        std::unique_ptr<geos::geom::Point> p0 = std::unique_ptr<geos::geom::Point>(linestring->getPointN(0));
        REQUIRE(3.2 == p0->getX());
    }

    SECTION("with duplicates and backwards") {
        std::unique_ptr<geos::geom::LineString> linestring {factory.create_linestring(wnl, osmium::geom::use_nodes::all, osmium::geom::direction::backward)};
        REQUIRE(4 == linestring->getNumPoints());
        std::unique_ptr<geos::geom::Point> p0 = std::unique_ptr<geos::geom::Point>(linestring->getPointN(0));
        REQUIRE(3.6 == p0->getX());
    }
}

TEST_CASE("GEOS geometry factory - create area with one outer and no inner rings") {
    osmium::geom::GEOSFactory<> factory;

    osmium::memory::Buffer buffer(10000);
    const osmium::Area& area = create_test_area_1outer_0inner(buffer);

    std::unique_ptr<geos::geom::MultiPolygon> mp {factory.create_multipolygon(area)};
    REQUIRE(1 == mp->getNumGeometries());

    const geos::geom::Polygon* p0 = dynamic_cast<const geos::geom::Polygon*>(mp->getGeometryN(0));
    REQUIRE(0 == p0->getNumInteriorRing());

    const geos::geom::LineString* l0e = p0->getExteriorRing();
    REQUIRE(4 == l0e->getNumPoints());

    std::unique_ptr<geos::geom::Point> l0e_p0 = std::unique_ptr<geos::geom::Point>(l0e->getPointN(1));
    REQUIRE(3.5 == l0e_p0->getX());
}

TEST_CASE("GEOS geometry factory - create area with one outer and one inner ring") {
    osmium::geom::GEOSFactory<> factory;

    osmium::memory::Buffer buffer(10000);
    const osmium::Area& area = create_test_area_1outer_1inner(buffer);

    std::unique_ptr<geos::geom::MultiPolygon> mp {factory.create_multipolygon(area)};
    REQUIRE(1 == mp->getNumGeometries());

    const geos::geom::Polygon* p0 = dynamic_cast<const geos::geom::Polygon*>(mp->getGeometryN(0));
    REQUIRE(1 == p0->getNumInteriorRing());

    const geos::geom::LineString* l0e = p0->getExteriorRing();
    REQUIRE(5 == l0e->getNumPoints());

    const geos::geom::LineString* l0i0 = p0->getInteriorRingN(0);
    REQUIRE(5 == l0i0->getNumPoints());
}

TEST_CASE("GEOS geometry factory - create area with two outer and two inner rings") {
    osmium::geom::GEOSFactory<> factory;

    osmium::memory::Buffer buffer(10000);
    const osmium::Area& area = create_test_area_2outer_2inner(buffer);

    std::unique_ptr<geos::geom::MultiPolygon> mp {factory.create_multipolygon(area)};
    REQUIRE(2 == mp->getNumGeometries());

    const geos::geom::Polygon* p0 = dynamic_cast<const geos::geom::Polygon*>(mp->getGeometryN(0));
    REQUIRE(2 == p0->getNumInteriorRing());

    const geos::geom::LineString* l0e = p0->getExteriorRing();
    REQUIRE(5 == l0e->getNumPoints());

    const geos::geom::Polygon* p1 = dynamic_cast<const geos::geom::Polygon*>(mp->getGeometryN(1));
    REQUIRE(0 == p1->getNumInteriorRing());

    const geos::geom::LineString* l1e = p1->getExteriorRing();
    REQUIRE(5 == l1e->getNumPoints());
}

