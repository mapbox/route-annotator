#include <cstddef>
#include <cstdlib>
#include <cstdio>

#include <string>
#include <vector>
#include <future>
#include <tuple>
#include <utility>
#include <iterator>
#include <stdexcept>
#include <system_error>
#include <iostream>
#include <algorithm>

#ifndef NDEBUG
#define BOOST_SPIRIT_X3_DEBUG
#endif

#include <boost/spirit/home/x3.hpp>

#include <cpprest/http_listener.h>
#include <cpprest/http_msg.h>
#include <cpprest/json.h>

#ifdef __unix__
#include <errno.h>
#include <signal.h>
#endif

#include <osmium/osm/types.hpp>
#include <osmium/handler.hpp>
#include <osmium/io/file.hpp>
#include <osmium/io/any_input.hpp>
#include <osmium/visitor.hpp>

#include <boost/iterator/zip_iterator.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/algorithm/string/replace.hpp>


typedef std::pair<std::uint64_t, std::uint64_t> nodepair_t;
typedef std::unordered_map<std::string,std::string> tagmap_t;

namespace std {
template <> struct hash<nodepair_t> {
    inline size_t operator()(const nodepair_t &v) const {
        return (v.first << 12) + v.second;
    }
};
}

struct Extractor final : osmium::handler::Handler {


    std::unordered_map<nodepair_t, std::uint32_t> pair_way_map;
    std::unordered_map<std::uint32_t, tagmap_t> way_tag_map;

    void way(const osmium::Way& way) {

        // TODO: check if way is interesting (road, highway, etc), there's no need to keep
        // everything....
        const char *oneway = way.tags().get_value_by_key("one_way");
        bool forward = (!oneway || std::string(oneway) == "yes");
        bool reverse = (!oneway || std::string(oneway) == "-1");
        if (way.nodes().size() > 1 && (forward || reverse))
        {
            std::for_each(
                boost::make_zip_iterator(boost::make_tuple(way.nodes().cbegin(), way.nodes().cbegin()+1)),
                boost::make_zip_iterator(boost::make_tuple(way.nodes().cend()-1, way.nodes().cend())),
                [this,&way,&forward,&reverse](boost::tuple<const osmium::NodeRef, const osmium::NodeRef> pair) {
                    if (forward) pair_way_map.insert({std::make_pair(pair.get<0>().ref(), pair.get<1>().ref()),way.id()});
                    if (reverse) pair_way_map.insert({std::make_pair(pair.get<1>().ref(), pair.get<0>().ref()),way.id()});
                }
            );

            tagmap_t tmp;

            for (const osmium::Tag& tag : way.tags()) {
                tmp.insert({tag.key(), tag.value()});
            }
            way_tag_map.insert({way.id(),tmp});
        }
    }
};

// Fulfilled promise signals clean shutdown
static std::promise<void> signal_shutdown;

int main(int argc, char** argv) try {
  if (argc != 3) {
    std::fprintf(stderr, "Usage: %s uri <filename.[pbf|osm|osm.bz2|osm.gz]>\n", argv[0]);
    return EXIT_FAILURE;
  }

#ifdef __unix__
  // Register CTRL+C handler on POSIX systems for clean shutdown
  struct ::sigaction action;
  ::sigemptyset(&action.sa_mask);
  action.sa_flags = SA_SIGINFO;
  action.sa_sigaction = [](int, ::siginfo_t*, void*) { signal_shutdown.set_value(); };

  if (::sigaction(SIGINT, &action, nullptr) == -1)
    throw std::system_error{errno, std::system_category()};
#endif

  // Load OSM file
  osmium::io::File osmfile{argv[2]};
  osmium::io::Reader fileReader(osmfile, osmium::osm_entity_bits::way | osmium::osm_entity_bits::node);

  Extractor extractor;
  osmium::apply(fileReader, extractor);

  std::cout << "Number of node pairs indexed: " << extractor.pair_way_map.size() << "\n";
  std::cout << "Number of ways indexed: " << extractor.way_tag_map.size() << "\n";

  // Server listens based on URI (Host, Port)
  web::uri uri{argv[1]};
  web::http::experimental::listener::http_listener listener{uri};

  // GET Request Handler
  listener.support(web::http::methods::GET, [&extractor](const auto& request) {
    const auto uri = request.relative_uri();
    const auto path = uri.path();
    const auto query = uri.query();

    std::fprintf(stderr, "%s\t%s\t%s\n", request.method().c_str(), path.c_str(), query.c_str());

    // Parse Coordinates with X3
    const auto parser = "/nodelist/" >> (boost::spirit::x3::int_) % ",";
    std::vector<std::uint64_t> nodeids;

    if (!parse(begin(path), end(path), parser, nodeids) || nodeids.size() < 2) {
      web::json::value response;
      response["message"] = web::json::value("Bad request");
      request.reply(web::http::status_codes::BadRequest,response);
      return; // Early Exit
    }

    web::json::value response;

    std::for_each(
        boost::make_zip_iterator(boost::make_tuple(nodeids.cbegin(), nodeids.cbegin()+1)),
        boost::make_zip_iterator(boost::make_tuple(nodeids.cend()-1, nodeids.cend())),
        [&extractor,&response](boost::tuple<const std::uint64_t, const std::uint64_t> pair) {
            if (extractor.pair_way_map.find(std::make_pair(pair.get<0>(), pair.get<1>())) != extractor.pair_way_map.end())
            {
                const auto way = extractor.pair_way_map[std::make_pair(pair.get<0>(), pair.get<1>())];
                const auto tagmap = extractor.way_tag_map[way];

                std::string nodepairstr(std::to_string(pair.get<0>()) + "," + std::to_string(pair.get<1>()));

                web::json::value tags = web::json::value::object();
                for (const auto &entry : tagmap) {
                    tags[entry.first] = web::json::value(entry.second);
                }
                response[nodepairstr] = tags;
            }
        }
    );

    request.reply(web::http::status_codes::OK, response);
  });


  // Start up server, handles concurrency internally
  listener.open().then([&uri] { std::fprintf(stderr, "Host %s\nPort %d\n\n", uri.host().c_str(), uri.port()); }).wait();

  // Main thread blocks on future until its associated promise is fulfilled from within the CTRL+C handler
  signal_shutdown.get_future().wait();

  // Only then we shutdown the server
  listener.close().then([] { std::fprintf(stderr, "\nBye!\n"); }).wait();

} catch (const std::exception& e) {
  std::fprintf(stderr, "Error: %s\n", e.what());
  return EXIT_FAILURE;
}
