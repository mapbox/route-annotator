#!/usr/bin/env bash

set -e
set -o pipefail

if [ ! -d src ] ; then
  echo "You need to run this from the root of your repository"
  exit 1
fi

docker build \
    -t mapbox/route-annotator:linux \
    docker/

