cmake_minimum_required (VERSION 3.1)
project (BerryDB)

# Update include/berrydb/version.h when changing the numbers below.
set (BerryDB_VERSION_MAJOR 0)
set (BerryDB_VERSION_MINOR 1)
set (BerryDB_VERSION_PATCH 0)

# This project requires C++ 11.
set (CMAKE_CXX_STANDARD 11)

configure_file (
  "${PROJECT_SOURCE_DIR}/src/version_unittest.cc.in"
  "${PROJECT_BINARY_DIR}/src/version_unittest.cc"
)

include_directories("${PROJECT_SOURCE_DIR}/include")

option (BUILD_TESTS
  "Build BerryDB's unit tests" ON)

add_library (berrydb
  include/berrydb.h
  include/berrydb/database.h
  include/berrydb/manager.h
  include/berrydb/version.h
  src/database.cc
  )

if (BUILD_TESTS)
  enable_testing()

  # This project is tested using GoogleTest.
  add_subdirectory (third_party/googletest)

  add_executable (berrydb_tests
    "${PROJECT_BINARY_DIR}/src/version_unittest.cc"
    src/database_unittest.cc
    )

  target_link_libraries(berrydb_tests berrydb gtest_main)

  add_test (NAME berrydb_tests COMMAND berrydb_tests)
endif (BUILD_TESTS)
