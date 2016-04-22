#!/usr/bin/env bash

set -e
set -o pipefail

if [ ! -d src ] ; then
  echo "You need to run this from the root directory of your repository"
  exit 1
fi

docker run \
    --interactive \
    --volume=`pwd`:/home/mapbox/route-annotator \
    --tty \
    --publish=5000:5000 \
    --workdir=/home/mapbox/route-annotator \
    mapbox/route-annotator:linux \
    /bin/bash -lc "node ./example-server.js monaco-latest.osm.pbf"
