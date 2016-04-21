#!/usr/bin/env bash

set -e
set -o pipefail

if [ ! -d src ] ; then
  echo "You need to run this from the root directory of your repository"
  exit 1
fi

docker run \
    --interactive \
    --env="CXX=g++-5" \
    --env="CC=gcc-5" \
    --volume=`pwd`:/home/mapbox/route-annotator \
    --tty \
    --workdir=/home/mapbox/route-annotator \
    mapbox/route-annotator:linux \
    /bin/bash -c "source /opt/nvm/nvm.sh && npm run build"
