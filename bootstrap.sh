#!/bin/bash

#set -e

if [[ `which pkg-config` ]]; then
    echo "Success: Found pkg-config";
else
    echo "echo you need pkg-config installed";
    exit 1;
fi;

function setup_mason() {
    if [[ ! -d ./.mason ]]; then
        git clone https://github.com/mapbox/mason.git ./.mason
        (cd ./.mason && git checkout ${MASON_VERSION})
    else
        echo "Updating to latest mason"
        (cd ./.mason && git fetch && git checkout ${MASON_VERSION})
    fi
    export MASON_DIR=$(pwd)/.mason
    export MASON_HOME=$(pwd)/mason_packages/.link
    export PATH=$(pwd)/.mason:$PATH
    export CXX=${CXX:-clang++-3.6}
    export CC=${CC:-clang-3.6}
}

function dep() {
    ./.mason/mason install $1 $2
    ./.mason/mason link $1 $2
}

export CXX=${CXX:-clang++-3.6}
export BUILD_TYPE=${BUILD_TYPE:-Release}

echo
echo "*******************"
echo -e "BUILD_TYPE set to:     \033[1m\033[36m ${BUILD_TYPE}\033[0m"
echo -e "CXX set to:            \033[1m\033[36m ${CXX}\033[0m"
echo "*******************"
echo
echo

function all_deps() {
    dep cmake 3.2.2 &
    dep boost 1.59.0 &
    dep boost_libeverything 1.59.0 &
    dep expat 2.1.0 &
    dep bzip 1.0.6 &
    dep zlib system &
    wait
}

function main() {
    setup_mason
    all_deps
    export PKG_CONFIG_PATH=${MASON_HOME}/lib/pkgconfig

    # environment variables to tell the compiler and linker
    # to prefer mason paths over other paths when finding
    # headers and libraries. This should allow the build to
    # work even when conflicting versions of dependencies
    # exist on global paths
    # stopgap until c++17 :) (http://www.open-std.org/JTC1/SC22/WG21/docs/papers/2014/n4214.pdf)
    export C_INCLUDE_PATH="${MASON_HOME}/include"
    export CPLUS_INCLUDE_PATH="${MASON_HOME}/include"
    export LIBRARY_PATH="${MASON_HOME}/lib"
    export PATH="${MASON_HOME}/bin:${PATH}"
    export BOOST_INCLUDEDIR="${MASON_HOME}/include"

    LINK_FLAGS=""
    if [[ $(uname -s) == 'Linux' ]]; then
        LINK_FLAGS="${LINK_FLAGS} "'-Wl,-z,origin -Wl,-rpath=\$ORIGIN'
    fi

    
    mkdir third_party
    pushd third_party
    git clone --depth 1 --branch v2.8.0 https://github.com/Microsoft/cpprestsdk.git casablanca
    git clone --depth 1 --branch v2.6.1 https://github.com/osmcode/libosmium.git libosmium
    popd

    mkdir -p third_party/casablanca/Release/build
    pushd third_party/casablanca/Release/build 
    #sed -i -e 's/ -Werror -pedantic//g' ../src/CMakeLists.txt
    cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_SAMPLES=OFF -DBUILD_SHARED_LIBS=OFF -DBUILD_TESTS=OFF
    cmake --build .
    popd

    make VERBOSE=1
}

main
set +e

