CMAKE_MINIMUM_REQUIRED(VERSION 3.0.2 FATAL_ERROR)

PROJECT(route-annotator VERSION 0.0.1 LANGUAGES C CXX)

IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Weverything")
elseif(UNIX) # This is for OSX
  # Prefer a homebrew version of OpenSSL over the one in /usr/lib
  file(GLOB OPENSSL_ROOT_DIR /usr/local/Cellar/openssl/*)
  # Prefer the latest (make the latest one first)
  list(REVERSE OPENSSL_ROOT_DIR)
  # This should prevent linking against the system provided 0.9.8y
  set(_OPENSSL_VERSION "")

  SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework CoreFoundation -framework CoreServices -framework Security")
ENDIF()

SET(CMAKE_CXX_STANDARD 14)
SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)

FIND_PACKAGE(Threads REQUIRED)
FIND_PACKAGE(OpenSSL 1.0.1 REQUIRED)
FIND_PACKAGE(ZLIB)
FIND_PACKAGE(BZip2)
FIND_PACKAGE(EXPAT)

FIND_PACKAGE(Boost 1.59.0 REQUIRED system thread filesystem chrono)
ADD_DEFINITIONS(-DBOOST_ALL_DYN_LINK -DBOOST_SPIRIT_USE_PHOENIX_V3 -DBOOST_RESULT_OF_USE_DECLTYPE)

# External dependency: run ./deps.sh

SET(Casablanca_INCLUDE_DIRS "third_party/casablanca/Release/include")
SET(Casablanca_LIBRARIES_DIRS "third_party/casablanca/Release/build/Binaries")
SET(Casablanca_LIBRARIES cpprest ssl crypto)
LINK_DIRECTORIES(${Casablanca_LIBRARIES_DIRS})

SET(libosmium_INCLUDE_DIRS "third_party/libosmium/include")

ADD_EXECUTABLE(route-annotatord src/server.cpp src/annotator.cpp src/database.cpp src/extractor.cpp)
TARGET_LINK_LIBRARIES(route-annotatord ${Casablanca_LIBRARIES} ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${Casablanca_LIBRARIES} ${OPENSSL_LIBRARIES} ${ZLIB_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${EXPAT_LIBRARIES} ${BZIP2_LIBRARIES} ${THREADS_LIBRARIES})
TARGET_INCLUDE_DIRECTORIES(route-annotatord SYSTEM PUBLIC ${Casablanca_INCLUDE_DIRS} ${libosmium_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS} ${OPENSSL_INCLUDE_DIR} ${ZLIB_INCLUDE_DIR} ${EXPAT_INCLUDE_DIR} ${BZIP2_INCLUDE_DIR})

add_subdirectory(test)
