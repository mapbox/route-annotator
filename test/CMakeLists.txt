file(GLOB BasicTestSources
    basic-tests.cpp
    basic/*.cpp
    ../src/database.cpp
    ../src/annotator.cpp)

add_executable(basic-tests
	EXCLUDE_FROM_ALL
	${BasicTestSources})

# FindPackage below overwrites Boost_LIBRARIES
set(AllBoostLibrariesExceptUnitTest ${Boost_LIBRARIES})

find_package(Boost 1.49.0 REQUIRED COMPONENTS unit_test_framework)

if(NOT WIN32)
  add_definitions(-DBOOST_TEST_DYN_LINK)
endif()

# After the find_package call we got only the unit test library
set(BoostUnitTestLibrary ${Boost_LIBRARIES})

include_directories(SYSTEM ${Boost_INCLUDE_DIRS})

target_include_directories(basic-tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../src ../third_party/libosmium/include)

target_link_libraries(basic-tests ${BoostUnitTestLibrary})

add_custom_target(all-tests
	DEPENDS
	basic-tests)
