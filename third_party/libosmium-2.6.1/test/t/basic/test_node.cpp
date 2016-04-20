#include "catch.hpp"

#include <boost/crc.hpp>

#include <osmium/builder/attr.hpp>
#include <osmium/osm/crc.hpp>
#include <osmium/osm/node.hpp>

using namespace osmium::builder::attr;

TEST_CASE("Build node") {
    osmium::memory::Buffer buffer(10000);

    osmium::builder::add_node(buffer,
        _id(17),
        _version(3),
        _visible(true),
        _cid(333),
        _uid(21),
        _timestamp(time_t(123)),
        _user("foo"),
        _tag("amenity", "pub"),
        _tag("name", "OSM BAR"),
        _location(3.5, 4.7)
    );

    osmium::Node& node = buffer.get<osmium::Node>(0);

    REQUIRE(osmium::item_type::node == node.type());
    REQUIRE(node.type_is_in(osmium::osm_entity_bits::node));
    REQUIRE(node.type_is_in(osmium::osm_entity_bits::nwr));
    REQUIRE(17l == node.id());
    REQUIRE(17ul == node.positive_id());
    REQUIRE(3 == node.version());
    REQUIRE(true == node.visible());
    REQUIRE(false == node.deleted());
    REQUIRE(333 == node.changeset());
    REQUIRE(21 == node.uid());
    REQUIRE(std::string("foo") == node.user());
    REQUIRE(123 == uint32_t(node.timestamp()));
    REQUIRE(osmium::Location(3.5, 4.7) == node.location());
    REQUIRE(2 == node.tags().size());

    osmium::CRC<boost::crc_32_type> crc32;
    crc32.update(node);
    REQUIRE(crc32().checksum() == 0x7dc553f9);

    node.set_visible(false);
    REQUIRE(false == node.visible());
    REQUIRE(true == node.deleted());
}

TEST_CASE("default values for node attributes") {
    osmium::memory::Buffer buffer(10000);

    osmium::builder::add_node(buffer, _id(0));

    const osmium::Node& node = buffer.get<osmium::Node>(0);
    REQUIRE(0l == node.id());
    REQUIRE(0ul == node.positive_id());
    REQUIRE(0 == node.version());
    REQUIRE(true == node.visible());
    REQUIRE(0 == node.changeset());
    REQUIRE(0 == node.uid());
    REQUIRE(std::string("") == node.user());
    REQUIRE(0 == uint32_t(node.timestamp()));
    REQUIRE(osmium::Location() == node.location());
    REQUIRE(0 == node.tags().size());
}

TEST_CASE("set node attributes from strings") {
    osmium::memory::Buffer buffer(10000);

    osmium::builder::add_node(buffer, _id(0));

    osmium::Node& node = buffer.get<osmium::Node>(0);
    node.set_id("-17")
        .set_version("3")
        .set_visible(true)
        .set_changeset("333")
        .set_uid("21");

    REQUIRE(-17l == node.id());
    REQUIRE(17ul == node.positive_id());
    REQUIRE(3 == node.version());
    REQUIRE(true == node.visible());
    REQUIRE(333 == node.changeset());
    REQUIRE(21 == node.uid());
}

TEST_CASE("set large id") {
    osmium::memory::Buffer buffer(10000);

    int64_t id = 3000000000l;
    osmium::builder::add_node(buffer, _id(id));

    osmium::Node& node = buffer.get<osmium::Node>(0);
    REQUIRE(id == node.id());
    REQUIRE(static_cast<osmium::unsigned_object_id_type>(id) == node.positive_id());

    node.set_id(-id);
    REQUIRE(-id == node.id());
    REQUIRE(static_cast<osmium::unsigned_object_id_type>(id) == node.positive_id());
}

TEST_CASE("set tags on node") {
    osmium::memory::Buffer buffer(10000);

    osmium::builder::add_node(buffer,
        _user("foo"),
        _tag("amenity", "pub"),
        _tag("name", "OSM BAR")
    );

    const osmium::Node& node = buffer.get<osmium::Node>(0);
    REQUIRE(nullptr == node.tags().get_value_by_key("fail"));
    REQUIRE(std::string("pub") == node.tags().get_value_by_key("amenity"));
    REQUIRE(std::string("pub") == node.get_value_by_key("amenity"));

    REQUIRE(std::string("default") == node.tags().get_value_by_key("fail", "default"));
    REQUIRE(std::string("pub") == node.tags().get_value_by_key("amenity", "default"));
    REQUIRE(std::string("pub") == node.get_value_by_key("amenity", "default"));
}

