# Route Annotator

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

To run the tests (which run both the JS tests and the C++ tests):

```
make test
```

To run just the JS tests:

```
npm test
```

To run the C++ tests:

```
./build/Release/basic-tests
```

## Usage

This library contains two main modules: `Annotator` and `SpeedLookup`

### Annotator

The `Annotator()` object is for looking up OSM tag data from OSM node IDs or coordinates.

**Example:**
```
var taglookup = new (require('route_annotator')).Annotator();

// Lookup some nodes and find out which ways they were on,
// and what tags they had
taglookup.loadOSMExtract(path.join(__dirname,'data/winthrop.osm'), (err) => {
  if (err) throw err;
  var nodes = [50253600,50253602,50137292];
  taglookup.annotateRouteFromNodeIds(nodes, (err, wayIds) => {
    if (err) throw err;
    annotator.getAllTagsForWayId(wayIds[0], (err, tags) => {
      if (err) throw err;
      console.log(tags);
    });
  });
}); 


// Do the same thing, but this time use coordinates instead
// of node ids.  Internally, a radius search finds the closest
// node within 5m
taglookup.loadOSMExtract(path.join(__dirname,'data/winthrop.osm'), (err) => {
  if (err) throw err;
  var coords = [[-120.1872774,48.4715898],[-120.1882910,48.4725110]];
  taglookup.annotateRouteFromLonLats(coords, (err, wayIds) => {
    if (err) throw err;
    annotator.getAllTagsForWayId(wayIds[0], (err, tags) => {
      if (err) throw err;
      console.log(tags);
    });
  });
}); 

```

### SpeedLookup

The `SpeedLookup()` object is for loading segment speed information from CSV files, then looking it up quickly from an in-memory hashtable.

**Example:**
```
var speedlookup = new (require('route_annotator')).SpeedLookup();

// Loads example.csv, then looks up the pairs 123-124, 124-125, 125-126
// and prints the speeds for those segments (3 values) as comma-separated
// data
speedlookup.loadCSV("example.csv", (err) => {
  if (err) throw err;
  speedlookup.getRouteSpeeds([123,124,125,126],(err,results) => {
    if (err) throw err;
    console.log(results.join(","));
  });
});
```

The `loadCSV` method can also be passed an array of filenames.

---

## Example HTTP server

```
npm install
curl --remote-name http://download.geofabrik.de/europe/monaco-latest.osm.pbf
node example-server.js monaco-latest.osm.pbf
```

Then, in a new terminal, you should be able to do:

```
curl "http://localhost:5000/coordlist/7.422155,43.7368838;7.4230139,43.7369751"
```

#### Release

- `git checkout master`
- Update CHANGELOG.md
- Bump version in package.json
- `git commit -am "vx.y.z [publish binary]"` with Changelog list in commit message
- `git tag vx.y.z -a` with Changelog list in tag message
- `git push origin master; git push origin --tags`
- `npm publish`
