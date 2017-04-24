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

# https://github.com/izenecloud/cmake/blob/master/SetCompilerWarningAll.cmake
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  # Use the highest warning level for Visual Studio.
  set (CMAKE_CXX_WARNING_LEVEL 4)
  if (CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    STRING(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else (CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  endif (CMAKE_CXX_FLAGS MATCHES "/W[0-4]")

else (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  # use -Wall for clang and gcc
  if (NOT CMAKE_CXX_FLAGS MATCHES "-Wall$")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
  endif (NOT CMAKE_CXX_FLAGS MATCHES "-Wall$")

  # use -Werror for clang and gcc
  if (NOT CMAKE_CXX_FLAGS MATCHES "-Werror$")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
  endif (NOT CMAKE_CXX_FLAGS MATCHES "-Werror$")

endif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")


option (BUILD_TESTS
  "Build BerryDB's unit tests" ON)

add_library (berrydb "")
target_sources(berrydb
  PRIVATE
    "${PROJECT_SOURCE_DIR}/src/page.h"
    "${PROJECT_SOURCE_DIR}/src/page.cc"
    "${PROJECT_SOURCE_DIR}/src/page_pool.cc"
    "${PROJECT_SOURCE_DIR}/src/page_pool.h"
    "${PROJECT_SOURCE_DIR}/src/pool.cc"
    "${PROJECT_SOURCE_DIR}/src/pool_impl.cc"
    "${PROJECT_SOURCE_DIR}/src/pool_impl.h"
    "${PROJECT_SOURCE_DIR}/src/store.cc"
    "${PROJECT_SOURCE_DIR}/src/util/platform_allocator.h"
    "${PROJECT_SOURCE_DIR}/src/util/vfs_stubs.cc"
    "${PROJECT_SOURCE_DIR}/src/vfs/libc_vfs.cc"
  PUBLIC
    "${PROJECT_BINARY_DIR}/platform/berrydb/platform/config.h"
    "${PROJECT_SOURCE_DIR}/platform/berrydb/platform.h"
    "${PROJECT_SOURCE_DIR}/platform/berrydb/platform/alloc.h"
    "${PROJECT_SOURCE_DIR}/platform/berrydb/platform/dcheck.h"
    "${PROJECT_SOURCE_DIR}/platform/berrydb/platform/string_view.h"
    "${PROJECT_SOURCE_DIR}/platform/berrydb/platform/types.h"
    "${PROJECT_SOURCE_DIR}/include/berrydb.h"
    "${PROJECT_SOURCE_DIR}/include/berrydb/pool.h"
    "${PROJECT_SOURCE_DIR}/include/berrydb/space.h"
    "${PROJECT_SOURCE_DIR}/include/berrydb/status.h"
    "${PROJECT_SOURCE_DIR}/include/berrydb/store.h"
    "${PROJECT_SOURCE_DIR}/include/berrydb/transaction.h"
    "${PROJECT_SOURCE_DIR}/include/berrydb/version.h"
    "${PROJECT_SOURCE_DIR}/src/space.h"
  )

if (BUILD_TESTS)
  enable_testing()

  # Prevent overriding the parent project's compiler/linker settings on Windows.
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

  # This project is tested using GoogleTest.
  add_subdirectory (third_party/googletest)

  add_executable (berrydb_tests "")
  target_sources (berrydb_tests
    PRIVATE
      "${PROJECT_SOURCE_DIR}/src/embedder_tests/alloc_unittest.cc"
      "${PROJECT_SOURCE_DIR}/src/embedder_tests/string_view_unittest.cc"
      "${PROJECT_SOURCE_DIR}/src/embedder_tests/vfs_unittest.cc"
      "${PROJECT_SOURCE_DIR}/src/store_unittest.cc"
      "${PROJECT_SOURCE_DIR}/src/util/platform_allocator_unittest.cc"
      "${PROJECT_BINARY_DIR}/src/version_unittest.cc"
    )

  target_link_libraries(berrydb_tests berrydb gtest_main)

  add_test (NAME berrydb_tests COMMAND berrydb_tests)
endif (BUILD_TESTS)
