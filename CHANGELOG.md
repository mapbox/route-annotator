# Route Annotator releases

## Unreleased

## 0.2.2-dev.3
- Added support units in SegmentSpeedMap.  Data can be in the format of `NODE_A,NODE_B,UNIT,MAXSPEED` or `NODE_A,NODE_B,MAXSPEED`.  If units are expected, then all speed is coverted to kph.  If the unit is blank, it is assumed that the speed is in kph.  Added a limit to speeds.  We now will write out an error if > (invalid_speed - 1).  Changed segment_speed_t to uint8_t.  Added logic to convert mph to kph for maxspeed tag for OSM data.

## 0.2.1
- If tag is not found, filter by certain highway types by default.  (i.e., always add routable ways even though the requested tags don't exist.)

## 0.2.0
- Implemented a key-value lookup service that can be used for fast in-memory lookup of speed data for wayids.
- Refactored existing code.  speed_lookup_bindings -> segment_bindings

## 0.1.1
- Fixed code coverage environment and expanded test coverage
- Add support for Node 8 pre-published binaries

## 0.1.0
- Added filtering option based on a file of OSM tags
    * loadOSMExtract can optionally be given another path to a file containing OSM tags on which to filter data storage
    * loadOSMExtract now accepts an array of OSM data files in addition to a single string path

## 0.0.7
 - Performance improvements (faster load times)
    * Only parse and save node coordinates if we need to build the rtree
    * Use faster lookup table for checking valid tag names.
    * Only store node pairs once, flag whether storage is forward/backward.
    * Don't store external way IDs as strings - saves space, and faster to lookup
    * Don't use location index if there is no rtree being built.
    * Use simpler loop - zip_iterator came with a small performance hit.

## 0.0.6
 - Made RTree construction in the annotator database optional and exposed it in the node bindings as a configuration option in the Annotator object

## 0.0.5
 - Removed large unecessary test fixture from published npm module

## 0.0.4
 - Fixed bug of segfaulting if using the hashmap without loading CSVs first

## 0.0.3
 - Added SpeedLookup() to read CSV files and load a hashmap of NodeId pairs (key) and speeds (value)
 - Changed over build system to use mason

## 0.0.2
 - Added Annotator() for looking up OSM tag data from OSM node IDs or coordinates

## 0.0.1
 - First versioned release
