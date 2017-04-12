# Route Annotator

[![Build Status](https://travis-ci.org/mapbox/route-annotator.svg?branch=master)](https://travis-ci.org/mapbox/route-annotator)
[![codecov](https://codecov.io/gh/mapbox/route-annotator/branch/master/graph/badge.svg)](https://codecov.io/gh/mapbox/route-annotator)

This is a NodeJS module that indexes connected node pairs in OSM data, and allows you to query for
meta-data.  It's useful for retrieving tag information when you have geometry from your basemap.

## Requires

- Node >= 4

## Building

To install via binaries:

```
npm install
```

To install from source, first install a C++14 capable compiler then run:


```
make
```

## Testing

Run: `npm test`

To run the C++ standalone tests run:

```
./build/Release/cxx-tests
```

### Example

```
npm install
curl --remote-name http://download.geofabrik.de/europe/monaco-latest.osm.pbf
node example-server.js monaco-latest.osm.pbf
```

Then, in a new terminal, you should be able to do:

```
curl "http://localhost:5000/coordlist/7.422155,43.7368838;7.4230139,43.7369751"
```