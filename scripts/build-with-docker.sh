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
    --workdir=/home/mapbox/route-annotator \
    mapbox/route-annotator:linux \
    /bin/bash -lc "npm run cmake"
