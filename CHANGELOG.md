# Route Annotator releases

## Unreleased

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
