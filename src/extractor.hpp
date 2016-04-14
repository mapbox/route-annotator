#pragma once
// Basic libosmium includes
#include <osmium/handler.hpp>
#include <osmium/osm/types.hpp>

#include "database.hpp"
#include "types.hpp"

/**
 * The handler for libosmium.  This class basically contains one callback that's called by
 * libosmium as it parses our OSM file.
 *
 * Basically, it's responsible for populating the Database object with nodes, ways
 * and tags.
 */
struct Extractor final : osmium::handler::Handler
{
    /**
     * Constructs an extractor.  It requres a Database object it
     * can dump data into.
     *
     * @param d the Database object where everything will end up
     */
    Extractor(const std::string &osmfilename, Database &d);

    /**
     * Constructs an extractor from in-memory OSM XML data.
     * It requres a Database object it can dump data into.
     *
     * @param buffer a character buffer holding the osmium parseable data
     * @param buffersize the buffer size (duh)
     * @param format the format of the buffer for libosmium.  One of
     *     pbf, xml, opl, json, o5m, osm, osh or osc
     */
    Extractor(const char *buffer, std::size_t buffersize, const std::string &format, Database &d);

    /**
     * Osmium way handler - called once for each way.
     *
     * @param way the current way being processed
     */
    void way(const osmium::Way &way);

  private:
    // Internal reference to the db we're going to dump everything
    // into
    Database &db;
};
