cmake_minimum_required(VERSION 3.7.2)
# Update include/berrydb/version.h when changing the version number.
project(BerryDB VERSION 0.1.0 LANGUAGES C CXX)

# This project requires C++14.
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

configure_file(
  "src/api/version_unittest.cc.in"
  "${PROJECT_BINARY_DIR}/src/api/version_unittest.cc"
)

# https://github.com/izenecloud/cmake/blob/master/SetCompilerWarningAll.cmake
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  # Use the highest warning level for Visual Studio.
  set(CMAKE_CXX_WARNING_LEVEL 4)
  if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  endif(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")

  # Disable C++ exceptions.
  string(REGEX REPLACE "/EH[a-z]+" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHs-c-")
  add_definitions(-D_HAS_EXCEPTIONS=0)

  # Disable RTTI.
  string(REGEX REPLACE "/GR" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GR-")
else(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  # Use -Wall for clang and gcc.
  if(NOT CMAKE_CXX_FLAGS MATCHES "-Wall")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
  endif(NOT CMAKE_CXX_FLAGS MATCHES "-Wall")

  # Use -Wextra for clang and gcc.
  if(NOT CMAKE_CXX_FLAGS MATCHES "-Wextra")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra")
  endif(NOT CMAKE_CXX_FLAGS MATCHES "-Wextra")

  # Use -Werror for clang and gcc.
  if(NOT CMAKE_CXX_FLAGS MATCHES "-Werror")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
  endif(NOT CMAKE_CXX_FLAGS MATCHES "-Werror")

  # Disable C++ exceptions.
  string(REGEX REPLACE "-fexceptions" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")

  # Disable RTTI.
  string(REGEX REPLACE "-frtti" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
endif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")

option(BERRYDB_BUILD_TESTS "Build BerryDB's unit tests" ON)
option(BERRYDB_BUILD_CLI "Build CLI for demo" ON)
option(BERRYDB_BUILD_BENCHMARKS "Build BerryDB's benchmarks" ON)
option(BERRYDB_USE_GLOG "Build with Google Logging" ON)

include(CheckCXXCompilerFlag)
# Used by glog.
check_cxx_compiler_flag(-Wno-deprecated BERRYDB_HAVE_NO_DEPRECATED)
# Used by glog and snappy.
check_cxx_compiler_flag(-Wno-sign-compare BERRYDB_HAVE_NO_SIGN_COMPARE)
# Used by glog and snappy.
check_cxx_compiler_flag(-Wno-unused-parameter BERRYDB_HAVE_NO_UNUSED_PARAMETER)
# Used by googletest.
check_cxx_compiler_flag(-Wno-missing-field-initializers
                        BERRYDB_HAVE_NO_MISSING_FIELD_INITIALIZERS)

if(BERRYDB_USE_GLOG)
  # glog requires this setting to avoid using dynamic_cast.
  set(DISABLE_RTTI ON CACHE BOOL "" FORCE)

  # glog's test targets trigger deprecation warnings, and compiling them burns
  # CPU cycles on the CI.
  set(BUILD_TESTING_SAVED "${BUILD_TESTING}")
  set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
  add_subdirectory("third_party/glog" EXCLUDE_FROM_ALL)
  set(BUILD_TESTING "${BUILD_TESTING_SAVED}" CACHE BOOL "" FORCE)

  # glog triggers deprecation warnings on OSX.
  # https://github.com/google/glog/issues/185
  if(BERRYDB_HAVE_NO_DEPRECATED)
    set_property(TARGET glog APPEND PROPERTY COMPILE_OPTIONS -Wno-deprecated)
  endif(BERRYDB_HAVE_NO_DEPRECATED)

  # glog triggers sign comparison warnings on gcc.
  if(BERRYDB_HAVE_NO_SIGN_COMPARE)
    set_property(TARGET glog APPEND PROPERTY COMPILE_OPTIONS -Wno-sign-compare)
  endif(BERRYDB_HAVE_NO_SIGN_COMPARE)

  # glog triggers unused parameter warnings on clang.
  if(BERRYDB_HAVE_NO_UNUSED_PARAMETER)
    set_property(TARGET glog
                 APPEND PROPERTY COMPILE_OPTIONS -Wno-unused-parameter)
  endif(BERRYDB_HAVE_NO_UNUSED_PARAMETER)

  set(BERRYDB_PLATFORM_BUILT_WITH_GLOG 1)
endif(BERRYDB_USE_GLOG)

configure_file(
  "platform/berrydb/platform/config.h.in"
  "${PROJECT_BINARY_DIR}/platform/berrydb/platform/config.h"
)

add_library(berrydb "")
target_sources(berrydb
  PRIVATE
    "src/api/catalog.cc"
    "src/api/options.cc"
    "src/api/ostream_ops.cc"
    "src/api/pool.cc"
    "src/api/space.cc"
    "src/api/status.cc"
    "src/api/store.cc"
    "src/api/transaction.cc"
    "src/api/vfs.cc"
    "src/format/store_header.cc"
    "src/format/store_header.h"
    "src/page.cc"
    "src/page.h"
    "src/catalog_impl.cc"
    "src/catalog_impl.h"
    "src/free_page_list_format.cc"
    "src/free_page_list_format.h"
    "src/free_page_list.cc"
    "src/free_page_list.h"
    "src/free_page_manager.cc"
    "src/free_page_manager.h"
    "src/page_pool.cc"
    "src/page_pool.h"
    "src/pool_impl.cc"
    "src/pool_impl.h"
    "src/space_impl.cc"
    "src/space_impl.h"
    "src/store_impl.cc"
    "src/store_impl.h"
    "src/transaction_impl.cc"
    "src/transaction_impl.h"
    "src/util/endianness.h"
    "src/util/linked_list.h"
    "src/util/platform_allocator.h"
    "src/util/platform_deleter.h"
    "src/util/span_util.h"
    "src/util/unique_ptr.h"
    "src/vfs/libc_vfs.cc"
  PUBLIC
    "${PROJECT_BINARY_DIR}/platform/berrydb/platform/config.h"
    "${PROJECT_SOURCE_DIR}/platform/berrydb/platform.h"
    "${PROJECT_SOURCE_DIR}/platform/berrydb/platform/alloc.h"
    "${PROJECT_SOURCE_DIR}/platform/berrydb/platform/dcheck.h"
    "${PROJECT_SOURCE_DIR}/platform/berrydb/platform/compiler.h"
    "${PROJECT_SOURCE_DIR}/platform/berrydb/platform/endianness.h"
    "${PROJECT_SOURCE_DIR}/platform/berrydb/platform/hashing.h"
    "${PROJECT_SOURCE_DIR}/include/berrydb.h"
    "${PROJECT_SOURCE_DIR}/include/berrydb/catalog.h"
    "${PROJECT_SOURCE_DIR}/include/berrydb/options.h"
    "${PROJECT_SOURCE_DIR}/include/berrydb/ostream_ops.h"
    "${PROJECT_SOURCE_DIR}/include/berrydb/pool.h"
    "${PROJECT_SOURCE_DIR}/include/berrydb/space.h"
    "${PROJECT_SOURCE_DIR}/include/berrydb/span.h"
    "${PROJECT_SOURCE_DIR}/include/berrydb/status.h"
    "${PROJECT_SOURCE_DIR}/include/berrydb/store.h"
    "${PROJECT_SOURCE_DIR}/include/berrydb/transaction.h"
    "${PROJECT_SOURCE_DIR}/include/berrydb/types.h"
    "${PROJECT_SOURCE_DIR}/include/berrydb/version.h"
    "${PROJECT_SOURCE_DIR}/include/berrydb/vfs.h"
)

target_include_directories(berrydb
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# TODO(pwnall): These should be PRIVATE for the berrydb target.
include_directories(
  "platform"
  "${PROJECT_BINARY_DIR}/platform"
)

# Warnings as errors in Visual Studio for this project's targets.
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  set_property(TARGET berrydb APPEND PROPERTY COMPILE_OPTIONS "/WX")
endif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")

if(BERRYDB_USE_GLOG)
  target_link_libraries(berrydb glog)
endif(BERRYDB_USE_GLOG)

if(BERRYDB_BUILD_TESTS)
  enable_testing()

  # Prevent overriding the parent project's compiler/linker settings on Windows.
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

  # This project is tested using GoogleTest.
  add_subdirectory("third_party/googletest")

  # GoogleTest triggers a missing field initializers warning.
  if(BERRYDB_HAVE_NO_MISSING_FIELD_INITIALIZERS)
    set_property(TARGET gtest
        APPEND PROPERTY COMPILE_OPTIONS -Wno-missing-field-initializers)
    set_property(TARGET gmock
        APPEND PROPERTY COMPILE_OPTIONS -Wno-missing-field-initializers)
  endif(BERRYDB_HAVE_NO_MISSING_FIELD_INITIALIZERS)

  add_executable(berrydb_tests "")
  target_sources(berrydb_tests
    PRIVATE
      "src/api/ostream_ops_unittest.cc"
      "src/api/pool_unittest.cc"
      "src/api/span_unittest.cc"
      "src/api/status_unittest.cc"
      "src/api/store_unittest.cc"
      "${PROJECT_BINARY_DIR}/src/api/version_unittest.cc"
      "src/embedder_tests/alloc_unittest.cc"
      "src/embedder_tests/compiler_unittest.cc"
      "src/embedder_tests/dcheck_unittest.cc"
      "src/embedder_tests/endianness_unittest.cc"
      "src/embedder_tests/vfs_unittest.cc"
      "src/format/store_header_unittest.cc"
      "src/free_page_list_format_unittest.cc"
      "src/free_page_list_unittest.cc"
      "src/page_pool_unittest.cc"
      "src/page_unittest.cc"
      "src/store_impl_unittest.cc"
      "src/test/block_access_file_wrapper.cc"
      "src/test/block_access_file_wrapper.h"
      "src/test/file_deleter.cc"
      "src/test/file_deleter.h"
      "src/test/file_deleter_unittest.cc"
      "src/test/test_main.cc"
      "src/util/checks_unittest.cc"
      "src/util/endianness_unittest.cc"
      "src/util/linked_list_unittest.cc"
      "src/util/platform_allocator_unittest.cc"
      "src/util/platform_deleter_unittest.cc"
      "src/util/span_util_unittest.cc"
      "src/util/unique_ptr_unittest.cc"
  )
  target_link_libraries(berrydb_tests berrydb gtest)

  # Warnings as errors in Visual Studio for this project's targets.
  if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set_property(TARGET berrydb_tests APPEND PROPERTY COMPILE_OPTIONS "/WX")
  endif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")

  if(BERRYDB_USE_GLOG)
    target_link_libraries(berrydb_tests glog)
  endif(BERRYDB_USE_GLOG)

  add_test(NAME berrydb_tests COMMAND berrydb_tests)
endif(BERRYDB_BUILD_TESTS)

if(BERRYDB_BUILD_CLI)
  set(BOOST_ROOT "third_party/")

  add_executable(berrydb_cli "")
  target_include_directories(berrydb_cli
    PRIVATE
      "third_party/boost/assert/include"
      "third_party/boost/config/include"
      "third_party/boost/core/include"
      "third_party/boost/detail/include"
      "third_party/boost/iterator/include"
      "third_party/boost/mpl/include"
      "third_party/boost/preprocessor/include"
      "third_party/boost/static_assert/include"
      "third_party/boost/throw_exception/include"
      "third_party/boost/tokenizer/include"
      "third_party/boost/type_traits/include"
  )
  target_sources(berrydb_cli
    PRIVATE
      "src/cli/cli.cc"
  )
  target_link_libraries(berrydb_cli berrydb)

  # Warnings as errors in Visual Studio for this project's targets.
  if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set_property(TARGET berrydb_cli APPEND PROPERTY COMPILE_OPTIONS "/WX")
  endif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")

  if(BERRYDB_USE_GLOG)
    target_link_libraries(berrydb_cli glog)
  endif(BERRYDB_USE_GLOG)
endif(BERRYDB_BUILD_CLI)

if(BERRYDB_BUILD_BENCHMARKS)
  add_executable(berrydb_bench "")
  target_sources(berrydb_bench
    PRIVATE
      "src/bench/benchmark_main.cc"
      "src/bench/crc32c_benchmark.cc"
      "src/bench/snappy_benchmark.cc"
      "src/bench/vfs_benchmark.cc"
      "src/test/file_deleter.cc"
      "src/test/file_deleter.h"
  )
  target_link_libraries(berrydb_bench berrydb)

  # This project uses Google benchmark for benchmarking.
  set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
  set(BENCHMARK_ENABLE_EXCEPTIONS OFF CACHE BOOL "" FORCE)
  add_subdirectory("third_party/benchmark")
  target_link_libraries(berrydb_bench benchmark)

  # This project uses crc32c.
  set(CRC32C_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(CRC32C_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
  set(CRC32C_USE_GLOG OFF CACHE BOOL "" FORCE)
  add_subdirectory("third_party/crc32c")
  target_link_libraries(berrydb_bench crc32c)

  # This project uses snappy.
  set(SNAPPY_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  add_subdirectory("third_party/snappy")
  target_link_libraries(berrydb_bench snappy)

  # Snappy triggers sign comparison warnings on clang.
  if(BERRYDB_HAVE_NO_SIGN_COMPARE)
    set_property(TARGET snappy
                 APPEND PROPERTY COMPILE_OPTIONS -Wno-sign-compare)
  endif(BERRYDB_HAVE_NO_SIGN_COMPARE)

  # Snappy triggers unused parameter warnings on clang.
  if(BERRYDB_HAVE_NO_UNUSED_PARAMETER)
    set_property(TARGET snappy
                 APPEND PROPERTY COMPILE_OPTIONS -Wno-unused-parameter)
  endif(BERRYDB_HAVE_NO_UNUSED_PARAMETER)

  # Snappy does not plan to fix some MSVC warnings.
  if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(snappy PRIVATE
      "/wd4100"  # Unreferenced formal parameter.
      "/wd4244"  # Lossy conversion.
      "/wd4018"  # Signed/unsigned mismatch.
    )
  endif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")

  # This project uses LevelDB.
  set(LEVELDB_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(LEVELDB_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
  set(LEVELDB_INSTALL OFF CACHE BOOL "" FORCE)
  add_subdirectory("${PROJECT_SOURCE_DIR}/third_party/leveldb")
  target_link_libraries(berrydb_bench leveldb)

  # LevelDB triggers sign comparison warnings on clang.
  if(BERRYDB_HAVE_NO_SIGN_COMPARE)
    set_property(TARGET leveldb
                 APPEND PROPERTY COMPILE_OPTIONS -Wno-sign-compare)
  endif(BERRYDB_HAVE_NO_SIGN_COMPARE)

  # LevelDB triggers unused parameter warnings on clang.
  if(BERRYDB_HAVE_NO_UNUSED_PARAMETER)
    set_property(TARGET leveldb
                 APPEND PROPERTY COMPILE_OPTIONS -Wno-unused-parameter)
  endif(BERRYDB_HAVE_NO_UNUSED_PARAMETER)

  # LevelDB triggers fallthrough warnings on gcc.
  check_cxx_compiler_flag(-Wimplicit-fallthrough=0
                          BERRYDB_HAVE_IMPLICIT_FALLTHROUGH0)
  if(BERRYDB_HAVE_IMPLICIT_FALLTHROUGH0)
    set_property(TARGET leveldb
                 APPEND PROPERTY COMPILE_OPTIONS -Wimplicit-fallthrough=0)
  endif(BERRYDB_HAVE_IMPLICIT_FALLTHROUGH0)

  if(BERRYDB_USE_GLOG)
    target_link_libraries(berrydb_bench glog)
  endif(BERRYDB_USE_GLOG)

  # Warnings as errors in Visual Studio for this project's targets.
  if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set_property(TARGET berrydb_bench APPEND PROPERTY COMPILE_OPTIONS "/WX")
  endif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
endif(BERRYDB_BUILD_BENCHMARKS)
