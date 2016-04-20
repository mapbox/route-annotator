#include "catch.hpp"

#include <osmium/builder/attr.hpp>
#include <osmium/memory/buffer.hpp>
#include <osmium/osm/node_ref.hpp>
#include <osmium/osm/node_ref_list.hpp>

TEST_CASE("NodeRef") {

    SECTION("instantiation_with_default_parameters") {
        osmium::NodeRef node_ref;
        REQUIRE(node_ref.ref() == 0);
//    REQUIRE(!node_ref.has_location());
    }

    SECTION("instantiation_with_id") {
        osmium::NodeRef node_ref(7);
        REQUIRE(node_ref.ref() == 7);
    }

    SECTION("equality") {
        osmium::NodeRef node_ref1(7, { 1.2, 3.4 });
        osmium::NodeRef node_ref2(7, { 1.4, 3.1 });
        osmium::NodeRef node_ref3(9, { 1.2, 3.4 });
        REQUIRE(node_ref1 == node_ref2);
        REQUIRE(node_ref1 != node_ref3);
        REQUIRE(!osmium::location_equal()(node_ref1, node_ref2));
        REQUIRE(!osmium::location_equal()(node_ref2, node_ref3));
        REQUIRE(osmium::location_equal()(node_ref1, node_ref3));
    }

    SECTION("set_location") {
        osmium::NodeRef node_ref(7);
        REQUIRE(!node_ref.location().valid());
        REQUIRE(node_ref.location() == osmium::Location());
        node_ref.set_location(osmium::Location(13.5, -7.2));
        REQUIRE(node_ref.location().lon() == 13.5);
        REQUIRE(node_ref.location().valid());
    }

    SECTION("ordering") {
        osmium::NodeRef node_ref1(1, { 1.0, 3.0 });
        osmium::NodeRef node_ref2(2, { 1.4, 2.9 });
        osmium::NodeRef node_ref3(3, { 1.2, 3.0 });
        osmium::NodeRef node_ref4(4, { 1.2, 3.3 });

        REQUIRE(node_ref1 < node_ref2);
        REQUIRE(node_ref2 < node_ref3);
        REQUIRE(node_ref1 < node_ref3);
        REQUIRE(node_ref1 >= node_ref1);

        REQUIRE(osmium::location_less()(node_ref1, node_ref2));
        REQUIRE(!osmium::location_less()(node_ref2, node_ref3));
        REQUIRE(osmium::location_less()(node_ref1, node_ref3));
        REQUIRE(osmium::location_less()(node_ref3, node_ref4));
        REQUIRE(!osmium::location_less()(node_ref1, node_ref1));
    }

}

TEST_CASE("WayNodeList") {
    osmium::memory::Buffer buffer(1024);

    SECTION("Empty list") {
        {
            osmium::builder::WayNodeListBuilder builder(buffer);
        }
        REQUIRE(buffer.commit() == 0);
        REQUIRE(buffer.committed( )> 0);

        const osmium::WayNodeList& nrl = buffer.get<osmium::WayNodeList>(0);
        REQUIRE(nrl.empty());
        REQUIRE(nrl.size() == 0);
    }

    SECTION("Small area") {
        osmium::builder::add_way_node_list(buffer, osmium::builder::attr::_nodes({
            { 1, {0, 0}},
            { 2, {0, 1}},
            { 3, {1, 1}},
            { 4, {1, 0}},
            { 1, {0, 0}},
        }));

        const osmium::WayNodeList& nrl = buffer.get<osmium::WayNodeList>(0);
        REQUIRE_FALSE(nrl.empty());
        REQUIRE(nrl.size() == 5);
        REQUIRE(nrl.is_closed());
        REQUIRE(nrl.ends_have_same_id());
        REQUIRE(nrl.ends_have_same_location());
    }

    SECTION("Not an area") {
        osmium::builder::add_way_node_list(buffer, osmium::builder::attr::_nodes({
            { 1, {0, 0}},
            { 2, {1, 0}},
            { 1, {0, 0}},
        }));

        const osmium::WayNodeList& nrl = buffer.get<osmium::WayNodeList>(0);
        REQUIRE_FALSE(nrl.empty());
        REQUIRE(nrl.size() == 3);
        REQUIRE(nrl.is_closed());
        REQUIRE(nrl.ends_have_same_id());
        REQUIRE(nrl.ends_have_same_location());
    }

}

