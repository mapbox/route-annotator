
#include "catch.hpp"
#include "utils.hpp"

#include <string>

#include <osmium/builder/attr.hpp>
#include <osmium/builder/osm_object_builder.hpp>
#include <osmium/io/compression.hpp>
#include <osmium/io/detail/input_format.hpp>
#include <osmium/io/detail/queue_util.hpp>
#include <osmium/io/file_format.hpp>
#include <osmium/io/header.hpp>
#include <osmium/io/reader.hpp>
#include <osmium/thread/queue.hpp>
#include <osmium/thread/util.hpp>

class MockParser : public osmium::io::detail::Parser {

    std::string m_fail_in;

public:

    MockParser(osmium::io::detail::future_string_queue_type& input_queue,
               osmium::io::detail::future_buffer_queue_type& output_queue,
               std::promise<osmium::io::Header>& header_promise,
               osmium::osm_entity_bits::type read_types,
               const std::string& fail_in) :
        Parser(input_queue, output_queue, header_promise, read_types),
        m_fail_in(fail_in) {
    }

    void run() final {
        osmium::thread::set_thread_name("_osmium_mock_in");

        if (m_fail_in == "header") {
            throw std::runtime_error("error in header");
        }

        set_header_value(osmium::io::Header{});

        osmium::memory::Buffer buffer(1000);
        osmium::builder::add_node(buffer, osmium::builder::attr::_user("foo"));
        send_to_output_queue(std::move(buffer));

        if (m_fail_in == "read") {
            throw std::runtime_error("error in read");
        }
    }

}; // class MockParser

TEST_CASE("Test Reader using MockParser") {

    std::string fail_in;

    osmium::io::detail::ParserFactory::instance().register_parser(
        osmium::io::file_format::xml,
        [&](osmium::io::detail::future_string_queue_type& input_queue,
            osmium::io::detail::future_buffer_queue_type& output_queue,
            std::promise<osmium::io::Header>& header_promise,
            osmium::osm_entity_bits::type read_which_entities) {
        return std::unique_ptr<osmium::io::detail::Parser>(new MockParser(input_queue, output_queue, header_promise, read_which_entities, fail_in));
    });

    SECTION("no failure") {
        fail_in = "";
        osmium::io::Reader reader(with_data_dir("t/io/data.osm"));
        auto header = reader.header();
        REQUIRE(reader.read());
        REQUIRE(!reader.read());
        REQUIRE(reader.eof());
        reader.close();
    }

    SECTION("throw in header") {
        fail_in = "header";
        try {
            osmium::io::Reader reader(with_data_dir("t/io/data.osm"));
            reader.header();
        } catch (std::runtime_error& e) {
            REQUIRE(std::string{e.what()} == "error in header");
        }
    }

    SECTION("throw in read") {
        fail_in = "read";
        osmium::io::Reader reader(with_data_dir("t/io/data.osm"));
        reader.header();
        try {
            reader.read();
        } catch (std::runtime_error& e) {
            REQUIRE(std::string{e.what()} == "error in read");
        }
        reader.close();
    }

    SECTION("throw in user code") {
        fail_in = "";
        osmium::io::Reader reader(with_data_dir("t/io/data.osm"));
        reader.header();
        try {
            throw std::runtime_error("error in user code");
        } catch (std::runtime_error& e) {
            REQUIRE(std::string{e.what()} == "error in user code");
        }
        REQUIRE(reader.read());
        REQUIRE(!reader.read());
        REQUIRE(reader.eof());
        reader.close();
    }

}

