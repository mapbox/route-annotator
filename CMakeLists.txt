CMAKE_MINIMUM_REQUIRED(VERSION 3.1 FATAL_ERROR)

# Note: project name is used for Node.JS bindings (shared lib); must not contain e.g. "-"
PROJECT(route_annotator VERSION 0.0.1 LANGUAGES C CXX)

IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  # Prefer a homebrew version of OpenSSL over the one in /usr/lib
  FILE(GLOB OPENSSL_ROOT_DIR /usr/local/Cellar/openssl/*)
  # Prefer the latest (make the latest one first)
  LIST(REVERSE OPENSSL_ROOT_DIR)
  # This should prevent linking against the system provided 0.9.8y
  SET(_OPENSSL_VERSION "")

  SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework CoreFoundation -framework CoreServices -framework Security")
ELSE()
  MESSAGE(FATAL_ERROR "Unsupported system")
ENDIF()

SET(CMAKE_CXX_STANDARD 14)
SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)

FIND_PACKAGE(Threads REQUIRED)
FIND_PACKAGE(ZLIB)
FIND_PACKAGE(BZip2)
FIND_PACKAGE(EXPAT)

FIND_PACKAGE(Boost 1.58.0 REQUIRED system thread filesystem chrono)
ADD_DEFINITIONS(-DBOOST_ALL_DYN_LINK -DBOOST_SPIRIT_USE_PHOENIX_V3 -DBOOST_RESULT_OF_USE_DECLTYPE)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/third_party/libosmium-2.6.1/cmake")
SET(OSMIUM_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/libosmium-2.6.1/include")
FIND_PACKAGE(Osmium REQUIRED COMPONENTS io)


# Build libannotator.{so,a} for both the annotator daemon as well as our (optional) Node bindings
SET(LibAnnotatorSources	src/annotator.cpp src/database.cpp src/extractor.cpp)
ADD_LIBRARY(annotator ${LibAnnotatorSources})
TARGET_LINK_LIBRARIES(annotator ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${OSMIUM_LIBRARIES})
TARGET_INCLUDE_DIRECTORIES(annotator SYSTEM PUBLIC ${Boost_INCLUDE_DIRS} ${OSMIUM_INCLUDE_DIRS})
SET_PROPERTY(TARGET annotator PROPERTY POSITION_INDEPENDENT_CODE ON)

# Build Node.JS bindings, linking in libannotator.{so,a}
# Note: instead of invoking this directly use "npm install"
OPTION(NODEJS "Build Node.js Bindings" OFF)
IF(NODEJS)
  LIST(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/node_modules/node-cmake)
  FIND_PACKAGE(NodeJS)

  ADD_NODEJS_MODULE(${PROJECT_NAME} src/nodejs_bindings.cpp)

  TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} SYSTEM PUBLIC "src" ${Boost_INCLUDE_DIRS})
  TARGET_LINK_LIBRARIES(${PROJECT_NAME} annotator)
  # Note: although we set the global property, we need this for CMake to specify -std=c++14
  SET_PROPERTY(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 14)

  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter")

  MESSAGE(STATUS "Node.js include path: ${NodeJS_INCLUDE_PATHS}")
  MESSAGE(STATUS "Node.js libraries: ${NodeJS_LIBRARIES}")
ENDIF()


ADD_SUBDIRECTORY(test)
