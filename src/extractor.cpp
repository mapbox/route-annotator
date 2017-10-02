#include "extractor.hpp"

#include <osmium/handler.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/osm/types.hpp>

// Basic libosmium includes
#include <osmium/handler.hpp>
#include <osmium/io/file.hpp>
#include <osmium/osm/types.hpp>
#include <osmium/visitor.hpp>
// We take any input that libosmium supports (XML, PBF, osm.bz2, etc)
#include <osmium/io/any_input.hpp>

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

Extractor::Extractor(const std::string &osmfilename, Database &db) : db(db)
{
    std::cerr << "Parsing " << osmfilename << " ... " << std::flush;
    osmium::io::File osmfile{osmfilename};
    osmium::io::Reader fileReader(osmfile,
                                  osmium::osm_entity_bits::way | osmium::osm_entity_bits::node);
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
    std::cerr << "done\n";
    std::cerr << "Number of node pairs indexed: " << db.pair_way_map.size() << "\n";
    std::cerr << "Number of ways indexed: " << db.way_tag_ranges.size() << "\n";

    // TODO split up compact() method into compacting and rtree creation
    std::cerr << "Constructing RTree ... " << std::flush;
    db.compact();
    std::cerr << "done\n" << std::flush;
    db.dump();
}

Extractor::Extractor(const char *buffer,
                     std::size_t buffersize,
                     const std::string &format,
                     Database &db)
    : db(db)
{
    std::cerr << "Parsing OSM buffer in format " << format << " ... " << std::flush;
    osmium::io::File osmfile{buffer, buffersize, format};
    osmium::io::Reader fileReader(osmfile,
                                  osmium::osm_entity_bits::way | osmium::osm_entity_bits::node);
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
    std::cerr << "done\n";
    std::cerr << "Number of node pairs indexed: " << db.pair_way_map.size() << "\n";
    std::cerr << "Number of ways indexed: " << db.way_tag_ranges.size() << "\n";

    std::cerr << "Constructing RTree ... " << std::flush;
    db.compact();
    std::cerr << "done\n" << std::flush;
    db.dump();
}

void Extractor::way(const osmium::Way &way)
{

    // Figure out which directions we need to process
    const char *oneway = way.tags().get_value_by_key("one_way");
    bool forward = (!oneway || std::strcmp(oneway, "yes") == 0);
    bool reverse = (!oneway || std::strcmp(oneway, "-1") == 0);

    // Check to see if it's an interesting type of way.  We're only
    // interested in roads at the moment.
    const char *highway = way.tags().get_value_by_key("highway");
    bool usable =
        highway &&
        (std::strcmp(highway, "motorway") == 0 || std::strcmp(highway, "motorway_link") == 0 ||
         std::strcmp(highway, "trunk") == 0 || std::strcmp(highway, "trunk_link") == 0 ||
         std::strcmp(highway, "primary") == 0 || std::strcmp(highway, "primary_link") == 0 ||
         std::strcmp(highway, "secondary") == 0 || std::strcmp(highway, "secondary_link") == 0 ||
         std::strcmp(highway, "tertiary") == 0 || std::strcmp(highway, "tertiary_link") == 0 ||
         std::strcmp(highway, "residential") == 0 || std::strcmp(highway, "living_street") == 0 ||
         std::strcmp(highway, "unclassified") == 0 || std::strcmp(highway, "service") == 0 ||
         std::strcmp(highway, "ferry") == 0 || std::strcmp(highway, "movable") == 0 ||
         std::strcmp(highway, "shuttle_train") == 0 || std::strcmp(highway, "default") == 0);

    if (usable && way.nodes().size() > 1 && (forward || reverse))
    {

        BOOST_ASSERT(db.key_value_pairs.size() < std::numeric_limits<std::uint32_t>::max());
        const auto tagstart = static_cast<std::uint32_t>(db.key_value_pairs.size());
        // Create a map of the tags for this way, add the strings to the stringbuffer
        // and then add the tag map to the way map.
        for (const osmium::Tag &tag : way.tags())
        {
            const auto key_pos = db.addstring(tag.key());
            const auto val_pos = db.addstring(tag.value());
            db.key_value_pairs.emplace_back(key_pos, val_pos);
        }
        db.key_value_pairs.emplace_back(db.addstring("_way_id"),
                                        db.addstring(std::to_string(way.id()).c_str()));
        BOOST_ASSERT(db.key_value_pairs.size() < std::numeric_limits<std::uint32_t>::max());
        const auto tagend = static_cast<std::uint32_t>(db.key_value_pairs.size() - 1);
        db.way_tag_ranges.emplace_back(tagstart, tagend);

        BOOST_ASSERT(db.way_tag_ranges.size() < std::numeric_limits<wayid_t>::max());
        const auto way_id = static_cast<wayid_t>(db.way_tag_ranges.size() - 1);

        // This iterates over each pair of nodes.
        // Given the nodes 1,2,3,4,5,6
        // This loop will be called with (1,2), (2,3), (3,4), (4,5), (5, 6)
        std::for_each(
            boost::make_zip_iterator(
                boost::make_tuple(way.nodes().cbegin(), way.nodes().cbegin() + 1)),
            boost::make_zip_iterator(boost::make_tuple(way.nodes().cend() - 1, way.nodes().cend())),
            [this, way_id, forward,
             reverse](boost::tuple<const osmium::NodeRef, const osmium::NodeRef> pair) {
                try
                {
                    point_t a{pair.get<0>().location().lon(), pair.get<0>().location().lat()};
                    point_t b{pair.get<1>().location().lon(), pair.get<1>().location().lat()};

                    internal_nodeid_t internal_a_id = 0;
                    internal_nodeid_t internal_b_id = 0;

                    const auto tmp_a = db.external_internal_map.find(pair.get<0>().ref());

                    if (tmp_a == db.external_internal_map.end())
                    {
                        internal_a_id = static_cast<internal_nodeid_t>(db.used_nodes_list.size());
                        db.used_nodes_list.emplace_back(a, internal_a_id);
                        db.external_internal_map.emplace(pair.get<0>().ref(), internal_a_id);
                    }
                    else
                    {
                        internal_a_id = tmp_a->second;
                    }

                    const auto tmp_b = db.external_internal_map.find(pair.get<1>().ref());
                    if (tmp_b == db.external_internal_map.end())
                    {
                        internal_b_id = static_cast<internal_nodeid_t>(db.used_nodes_list.size());
                        db.used_nodes_list.emplace_back(b, internal_b_id);
                        db.external_internal_map.emplace(pair.get<1>().ref(), internal_b_id);
                    }
                    else
                    {
                        internal_b_id = tmp_b->second;
                    }

                    if (forward)
                    {
                        db.pair_way_map.emplace(std::make_pair(internal_a_id, internal_b_id),
                                                way_id);
                    }
                    if (reverse)
                    {
                        db.pair_way_map.emplace(std::make_pair(internal_b_id, internal_a_id),
                                                way_id);
                    }
                }
                catch (const osmium::invalid_location &e)
                {
                    // std::cerr << "WARNING: Invalid location for one of nodes " <<
                    // pair.get<0>().ref() << " or " << pair.get<1>().ref() << "\n";
                }
            });
    }
}
