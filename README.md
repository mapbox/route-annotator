# Route Annotator

This is a NodeJS module that indexes connected node pairs in OSM data, and allows you to query for
meta-data.  It's useful for retrieving tag information when you have geometry from your basemap.

## Building

### Using Docker

The easiest way to get all the correct dependencies installed is by using the supplied Docker container configuration.  This will create a Docker container that has everything needed to compile and run the examples.

Do:

```
./scripts/create-docker-image.sh
./scripts/build-with-docker.sh
curl --remote-name http://download.geofabrik.de/europe/monaco-latest.osm.pbf
./scripts/run-example-server.sh
```

Then, in a new terminal, you should be able to do:

```
curl "http://localhost:5000/coordlist/7.422155,43.7368838;7.4230139,43.7369751"
```

### Builing locally

*All*: You'll need NodeJS 4.x

*Linux:* See `apt-get` commands in the `Dockerfile`

*Homebrew:* `??? (brew install cmake git boost stxxl)`

Then:
```
npm install
curl --remote-name http://download.geofabrik.de/europe/monaco-latest.osm.pbf
node example-server.js monaco-latest.osm.pbf
```

Then in a new terminal, you should be able to do:

```
curl "http://localhost:5000/coordlist/7.422155,43.7368838;7.4230139,43.7369751"
```
