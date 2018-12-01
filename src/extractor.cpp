#include "extractor.hpp"

#include <osmium/handler.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/osm/types.hpp>

// Basic libosmium includes
#include <osmium/handler.hpp>
#include <osmium/io/file.hpp>
#include <osmium/osm/types.hpp>
#include <osmium/tags/matcher.hpp>
#include <osmium/visitor.hpp>

// Needed for lon/lat lookups inside way handler
#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/index/map/all.hpp>

// For iterating over pairs of nodes/coordinates
#include <boost/iterator/zip_iterator.hpp>
#include <boost/tuple/tuple.hpp>

// Node indexing types for libosmium
// We need these because we need access to the lon/lat for the noderefs inside the way
// callback in our handler.
typedef osmium::index::map::Dummy<osmium::unsigned_object_id_type, osmium::Location> index_neg_type;
typedef osmium::index::map::SparseMemArray<osmium::unsigned_object_id_type, osmium::Location>
    index_pos_type;
// typedef osmium::index::map::DenseMmapArray<osmium::unsigned_object_id_type, osmium::Location>
// index_pos_type;
typedef osmium::handler::NodeLocationsForWays<index_pos_type, index_neg_type> location_handler_type;

void Extractor::ParseTags(std::ifstream &tagfile)
{
    std::string line;
    while (std::getline(tagfile, line))
    {
        tags_filter.add_rule(true, osmium::TagMatcher(line));
    }
}

void Extractor::SetupDatabase()
{
    if (db.createRTree)
    {
        std::cout << "Constructing RTree ... " << std::flush;
        db.build_rtree();
    }
    db.compact();
    std::cout << "done\n" << std::flush;
    db.dump();
}

void Extractor::ParseFile(const osmium::io::File &osmfile)
{
    osmium::io::Reader fileReader(osmfile, osmium::osm_entity_bits::way |
                                               (db.createRTree ? osmium::osm_entity_bits::node
                                                               : osmium::osm_entity_bits::nothing));
    if (db.createRTree)
    {
        int fd = open("nodes.cache", O_RDWR | O_CREAT, 0666);
        if (fd == -1)
        {
            throw std::runtime_error(strerror(errno));
        }
        index_pos_type index_pos{fd};
        index_neg_type index_neg;
        location_handler_type location_handler(index_pos, index_neg);
        location_handler.ignore_errors();
        osmium::apply(fileReader, location_handler, *this);
    }
    else
    {
        osmium::apply(fileReader, *this);
    }
    std::cout << "done\n";
    std::cout << "Number of node pairs indexed: " << db.pair_way_map.size() << "\n";
    std::cout << "Number of ways indexed: " << db.way_tag_ranges.size() << "\n";
}

Extractor::Extractor(const std::vector<std::string> &osm_files, Database &db) : db(db)
{
    for (const std::string &file : osm_files)
    {
        std::cout << "Parsing " << file << " ... " << std::flush;
        osmium::io::File osmfile{file};
        ParseFile(osmfile);
    }
    SetupDatabase();
}

Extractor::Extractor(const std::vector<std::string> &osm_files,
                     Database &db,
                     const std::string &tagfilename)
    : db(db)
{
    // add tags to tag filter object for use in way parsing
    if (!tagfilename.empty())
    {
        std::cout << "Parsing " << tagfilename << " ... " << std::flush;
        std::ifstream tagfile(tagfilename);
        if (!tagfile.is_open())
        {
            throw std::runtime_error(strerror(errno));
        }
        ParseTags(tagfile);
    }
    for (const std::string &file : osm_files)
    {
        std::cout << "Parsing " << file << " ... " << std::flush;
        osmium::io::File osmfile{file};
        ParseFile(osmfile);
    }
    SetupDatabase();
}

Extractor::Extractor(const char *buffer,
                     std::size_t buffersize,
                     const std::string &format,
                     Database &db)
    : db(db)
{
    std::cout << "Parsing OSM buffer in format " << format << " ... " << std::flush;
    osmium::io::File osmfile{buffer, buffersize, format};
    ParseFile(osmfile);
    SetupDatabase();
}

bool Extractor::FilterWay(const osmium::Way &way)
{
    if (tags_filter.empty())
    {
        // if not filtering by tags, filter by certain highway types by default
        const char *highway = way.tags().get_value_by_key("highway");
        std::vector<const char *> highway_types = {
            "motorway",     "motorway_link", "trunk",          "trunk_link", "primary",
            "primary_link", "secondary",     "secondary_link", "tertiary",   "tertiary_link",
            "residential",  "living_street", "unclassified",   "service",    "ferry",
            "movable",      "shuttle_train", "default"};
        return highway &&
               std::any_of(highway_types.begin(), highway_types.end(),
                           [&highway](const auto &way) { return std::strcmp(highway, way) == 0; });
    }
    else
    {
        for (auto &tag : way.tags())
        {
            // use this way if we find a tag that we're interested in
            if (tags_filter(tag))
            {
                return true;
            }
        }
        return false;
    }
}

void Extractor::way(const osmium::Way &way)
{

    // Check if the way contains tags we are interested in
    const bool usable = FilterWay(way);

    if (usable && way.nodes().size() > 1)
    {
        BOOST_ASSERT(db.key_value_pairs.size() < std::numeric_limits<std::uint32_t>::max());
        const auto tagstart = static_cast<std::uint32_t>(db.key_value_pairs.size());
        // Create a map of the tags for this way, add the strings to the stringbuffer
        // and then add the tag map to the way map.
        for (auto &tag : way.tags())
        {
            // use this way if we find a tag that we're interested in
            if (tags_filter(tag))
            {
                const auto key_pos = db.addstring(tag.key());
                const auto val_pos = db.addstring(tag.value());
                db.key_value_pairs.emplace_back(key_pos, val_pos);
            }
        }

        BOOST_ASSERT(db.key_value_pairs.size() < std::numeric_limits<std::uint32_t>::max());
        const auto tagend = static_cast<std::uint32_t>(db.key_value_pairs.size());
        db.way_tag_ranges.emplace_back(tagstart, tagend);

        BOOST_ASSERT(db.way_tag_ranges.size() < std::numeric_limits<wayid_t>::max());
        const auto way_id =
            static_cast<wayid_t>(db.way_tag_ranges.empty() ? 0 : (db.way_tag_ranges.size() - 1));
        db.internal_to_external_way_id_map.push_back(way.id());

        if (way.id() == 149731570)
          std::cout << "import " << way_id << " " << db.internal_to_external_way_id_map.size() << std::endl;


        // This iterates over each pair of nodes.
        // Given the nodes 1,2,3,4,5,6
        // This loop will be called with (1,2), (2,3), (3,4), (4,5), (5, 6)
        for (auto n = way.nodes().cbegin(); n != way.nodes().cend() - 1; n++)
        {
            const auto external_a = n;
            const auto external_b = n + 1;
            const auto external_a_ref = external_a->ref();
            const auto external_b_ref = external_b->ref();
            try
            {

                internal_nodeid_t internal_a_id;
                internal_nodeid_t internal_b_id;

                const auto tmp_a = db.external_internal_map.find(external_a_ref);

                if (tmp_a == db.external_internal_map.end())
                {
                    internal_a_id = db.external_internal_map.size();
                    if (db.createRTree)
                    {
                        point_t a{external_a->location().lon(), external_a->location().lat()};
                        BOOST_ASSERT(db.used_nodes_list.size() == db.external_internal_map.size());
                        db.used_nodes_list.emplace_back(a, internal_a_id);
                    }
                    db.external_internal_map.emplace(external_a_ref, internal_a_id);
                }
                else
                {
                    internal_a_id = tmp_a->second;
                }

                const auto tmp_b = db.external_internal_map.find(external_b_ref);
                if (tmp_b == db.external_internal_map.end())
                {
                    internal_b_id = db.external_internal_map.size();
                    if (db.createRTree)
                    {
                        point_t b{external_b->location().lon(), external_b->location().lat()};
                        BOOST_ASSERT(db.used_nodes_list.size() == db.external_internal_map.size());
                        db.used_nodes_list.emplace_back(b, internal_b_id);
                    }
                    db.external_internal_map.emplace(external_b_ref, internal_b_id);
                }
                else
                {
                    internal_b_id = tmp_b->second;
                }

                if (internal_a_id < internal_b_id)
                {
                    // true here indicates storage is forward
                    db.pair_way_map.emplace(std::make_pair(internal_a_id, internal_b_id),
                                            way_storage_t{way_id, true});
                }
                else
                {
                    // false here indicates storage is backward
                    db.pair_way_map.emplace(std::make_pair(internal_b_id, internal_a_id),
                                            way_storage_t{way_id, false});
                }
            }
            catch (const osmium::invalid_location &e)
            {
                // std::cerr << "WARNING: Invalid location for one of nodes " <<
                // external_a_ref << " or " << external_b_ref << "\n";
            }
        }
    }
}
