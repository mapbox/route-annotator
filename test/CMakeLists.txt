file(GLOB BasicTestSources
    basic-tests.cpp
    basic/*.cpp
    ../src/database.cpp
    ../src/annotator.cpp
    ../src/extractor.cpp)

add_executable(basic-tests
	EXCLUDE_FROM_ALL
	${BasicTestSources})

# FindPackage below overwrites Boost_LIBRARIES
set(AllBoostLibrariesExceptUnitTest ${Boost_LIBRARIES})

find_package(Boost 1.58.0 REQUIRED COMPONENTS unit_test_framework)
FIND_PACKAGE(Threads REQUIRED)
FIND_PACKAGE(ZLIB)
FIND_PACKAGE(BZip2)
FIND_PACKAGE(EXPAT)

if(NOT WIN32)
  add_definitions(-DBOOST_TEST_DYN_LINK)
endif()

# After the find_package call we got only the unit test library
set(BoostUnitTestLibrary ${Boost_LIBRARIES})

include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
SET(libosmium_INCLUDE_DIRS "../third_party/libosmium-2.6.1/include")

target_include_directories(basic-tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../src ${libosmium_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS} ${ZLIB_INCLUDE_DIR} ${EXPAT_INCLUDE_DIR} ${BZIP2_INCLUDE_DIR})

target_link_libraries(basic-tests ${BoostUnitTestLibrary} ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${ZLIB_LIBRARIES} ${EXPAT_LIBRARIES} ${BZIP2_LIBRARIES} ${THREADS_LIBRARIES})
add_custom_target(all-tests
	DEPENDS
	basic-tests)
