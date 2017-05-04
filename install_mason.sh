#!/bin/bash

set -eu
set -o pipefail

function install() {
  mason install $1 $2
  mason link $1 $2
}

# setup mason
./scripts/setup.sh --config local.env
source local.env

install boost 1.63.0
install boost_libtest 1.63.0
install boost_libiostreams 1.63.0
install libosmium 2.12.0
install expat 2.2.0
install bzip2 1.0.6
install protozero 1.5.1
install utfcpp 2.3.4
install zlib 1.2.8
install sparsepp 0.95
install clang-format 3.8.1
