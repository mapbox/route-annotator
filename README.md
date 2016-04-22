# Route Annotator

This is a node module that indexes connected node pairs in OSM data, and allows you to query for
meta-data.  It's useful for retrieving tag information when you have geometry from your basemap.

## Building

```
npm install
```

## Run the example server

```
curl --remote-name http://download.geofabrik.de/europe/monaco-latest.osm.pbf
node example-server.js monaco-latest.osm.pbf
```

## Get an example response

```
curl "http://localhost:5000/coordlist/7.422155,43.7368838;7.4230139,43.7369751"
```
