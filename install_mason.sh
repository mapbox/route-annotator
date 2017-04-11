#!/bin/bash

set -eu
set -o pipefail

function install() {
    ./mason/mason install $1 $2
    ./mason/mason link $1 $2
}

if [ ! -f ./mason/mason.sh ]; then
    mkdir -p ./mason
    curl -sSfL https://github.com/mapbox/mason/archive/v0.8.0.tar.gz | tar --gunzip --extract --strip-components=1 --exclude="*md" --exclude="test*" --directory=./mason
fi

install boost 1.63.0
install boost_libtest 1.63.0
install libosmium 2.12.0
install expat 2.2.0
install bzip2 1.0.6
install protozero 1.5.1
install utfcpp 2.3.4
install zlib 1.2.8