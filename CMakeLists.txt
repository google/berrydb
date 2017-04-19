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

include (CheckIncludeFiles)
check_include_files (string_view HAVE_STRING_VIEW)

configure_file (
  "${PROJECT_SOURCE_DIR}/platform/berrydb/platform/config.h.in"
  "${PROJECT_BINARY_DIR}/platform/berrydb/platform/config.h"
)

include_directories (
  "${PROJECT_SOURCE_DIR}/include"
  "${PROJECT_SOURCE_DIR}/platform"
  "${PROJECT_BINARY_DIR}/platform"
  )

option (BUILD_TESTS
  "Build BerryDB's unit tests" ON)

add_library (berrydb "")
target_sources(berrydb
  PRIVATE
    "${PROJECT_SOURCE_DIR}/src/database.cc"
  PUBLIC
    "${PROJECT_BINARY_DIR}/platform/berrydb/platform/config.h"
    "${PROJECT_SOURCE_DIR}/platform/berrydb/platform.h"
    "${PROJECT_SOURCE_DIR}/platform/berrydb/platform/dcheck.h"
    "${PROJECT_SOURCE_DIR}/platform/berrydb/platform/string_view.h"
    "${PROJECT_SOURCE_DIR}/platform/berrydb/platform/types.h"
    "${PROJECT_SOURCE_DIR}/include/berrydb.h"
    "${PROJECT_SOURCE_DIR}/include/berrydb/database.h"
    "${PROJECT_SOURCE_DIR}/include/berrydb/manager.h"
    "${PROJECT_SOURCE_DIR}/include/berrydb/version.h"
  )

if (BUILD_TESTS)
  enable_testing()

  # This project is tested using GoogleTest.
  add_subdirectory (third_party/googletest)

  add_executable (berrydb_tests "")
  target_sources (berrydb_tests
    PRIVATE
      "${PROJECT_SOURCE_DIR}/src/database_unittest.cc"
      "${PROJECT_SOURCE_DIR}/src/string_view_unittest.cc"
      "${PROJECT_BINARY_DIR}/src/version_unittest.cc"
    )

  target_link_libraries(berrydb_tests berrydb gtest_main)

  add_test (NAME berrydb_tests COMMAND berrydb_tests)
endif (BUILD_TESTS)
