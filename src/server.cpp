/************************************************************
 * Route Annotator HTTP server
 *
 * This HTTP service takes a list of OSM nodeids or coordinates,
 * and returns all the tags on the ways that those nodeids
 * touch.
 *
 * When using coordinates, they must be within 1m of the coordinates
 * loaded from the OSM file.
 ************************************************************/
#include <vector>
#include <future>

#ifndef NDEBUG
#define BOOST_SPIRIT_X3_DEBUG
#endif

#include <boost/spirit/home/x3.hpp>

#include <cpprest/http_listener.h>
#include <cpprest/http_msg.h>
#include <cpprest/json.h>

#include <errno.h>
#include <signal.h>

#include <boost/timer/timer.hpp>

#include "types.hpp"
#include "annotator.hpp"

// Fulfilled promise signals clean shutdown
static std::promise<void> signal_shutdown;

int main(int argc, char **argv) try
{
    if (argc != 3)
    {
        std::fprintf(stderr, "Usage: %s uri <filename.[pbf|osm|osm.bz2|osm.gz]>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Register CTRL+C handler on POSIX systems for clean shutdown
    struct ::sigaction action;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_SIGINFO;
    action.sa_sigaction = [](int, ::siginfo_t *, void *)
    {
        signal_shutdown.set_value();
    };

    if (::sigaction(SIGINT, &action, nullptr) == -1)
        throw std::system_error{errno, std::system_category()};

    // This is our callback handler, it ends up with all the data
    RouteAnnotator annotator(argv[2]);

    // Setup the web server
    // Server listens based on URI (Host, Port)
    web::uri uri{argv[1]};
    web::http::experimental::listener::http_listener listener{uri};

    // GET Request Handler
    listener.support(
        web::http::methods::GET, [&annotator](const auto &request)
        {
            const auto uri = request.relative_uri();
            const auto path = uri.path();
            const auto query = uri.query();

            std::vector<internal_nodeid_t> internal_nodeids;

            // Log the request
            std::fprintf(stderr, "%s\t%s\t%s\n", request.method().c_str(), path.c_str(),
                         query.c_str());

            // Step 1 - parse the URL
            if (path == "/")
            {
                request.reply(web::http::status_codes::OK);
                return;
            }
            // If a list of OSM node ids is supplied, convert it to internal nodeids
            else if (path.find("/nodelist/") == 0)
            {

                const auto nodelist_parser = "/nodelist/" >> (boost::spirit::x3::ulong_long) % ",";
                std::vector<external_nodeid_t> external_nodeids;

                if (!parse(begin(path), end(path), nodelist_parser, external_nodeids) ||
                    external_nodeids.size() < 2)
                {
                    web::json::value response;
                    response["message"] = web::json::value("Bad request");
                    request.reply(web::http::status_codes::BadRequest, response);
                    return;
                }

                internal_nodeids = annotator.external_to_internal(external_nodeids);
            } 
            // If a list of coordinates is supplied, convert it to a list of
            // internal node ids
            else if (path.find("/coordlist/") == 0)
            {
                const auto coordlist_parser =
                    "/coordlist/" >>
                    (boost::spirit::x3::double_ >> "," >> boost::spirit::x3::double_) % ";";
                std::vector<double> coordinates;

                if (!parse(begin(path), end(path), coordlist_parser, coordinates) ||
                    coordinates.size() < 2 || coordinates.size() % 2 != 0)
                {
                    web::json::value response;
                    response["message"] = web::json::value("Bad request");
                    request.reply(web::http::status_codes::BadRequest, response);
                    return; // Early Exit
                }

                std::vector<point_t> points;
                for (std::size_t lonIdx{0}, latIdx{1}; latIdx < coordinates.size();
                     lonIdx += 2, latIdx += 2)
                {
                    points.emplace_back(coordinates[lonIdx], coordinates[latIdx]);
                }

                internal_nodeids = annotator.coordinates_to_internal(points);
            }

            // If after all of the above, we still don't have at least two nodes, this is a bad
            // request
            if (internal_nodeids.size() < 2)
            {
                web::json::value response;
                response["message"] = web::json::value("Bad request");
                request.reply(web::http::status_codes::BadRequest, response);
            }


            // Do the actual annotation
            const auto annotated_route = annotator.annotateRoute(internal_nodeids);

            // Now, construct the response in JSON
            web::json::value response;

            web::json::value j_array = web::json::value::array(annotated_route.tagset_offset_list.size());
            int idx = 0;
            for (const auto &offset : annotated_route.tagset_offset_list)
            {
                j_array[idx++] = offset;
            }

            response["tagset_indexes"] = j_array;

            web::json::value waydata = web::json::value::array(annotated_route.tagset_list.size());

            idx = 0;
            for (const auto &tagset_id : annotated_route.tagset_list)
            {
                // Then get the tags for that way
                const auto tag_range = annotator.get_tag_range(tagset_id);

                web::json::value tags = web::json::value::object();
                for (auto i = tag_range.first; i <= tag_range.second; i++)
                {
                    tags[annotator.get_tag_key(i)] =
                        web::json::value(annotator.get_tag_value(i));
                }
                waydata[idx++] = tags;
            }

            response["way_tags"] = waydata;

            // Send the reply back
            request.reply(web::http::status_codes::OK, response);
        });

    // Start up server, handles concurrency internally
    listener.open()
        .then([&uri]
              {
                  std::fprintf(stderr, "Host %s\nPort %d\n\n", uri.host().c_str(), uri.port());
              })
        .wait();

    // Main thread blocks on future until its associated promise is fulfilled from within the CTRL+C
    // handler
    signal_shutdown.get_future().wait();

    // Only then we shutdown the server
    listener.close()
        .then([]
              {
                  std::fprintf(stderr, "\nBye!\n");
              })
        .wait();
}
catch (const std::exception &e)
{
    std::fprintf(stderr, "Error: %s\n", e.what());
    return EXIT_FAILURE;
}
