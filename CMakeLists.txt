cmake_minimum_required(VERSION 3.1)
# Update include/berrydb/version.h when changing the version number.
project(BerryDB VERSION 0.1.0 LANGUAGES C CXX)

# This project requires C++11.
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

configure_file(
  "${PROJECT_SOURCE_DIR}/src/api/version_unittest.cc.in"
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
  string(REGEX REPLACE "/EH[a-z]+" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")

  # Disable RTTI.
  string(REGEX REPLACE "/GR" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
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
  string(REGEX REPLACE "-fexceptions" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")

  # Disable RTTI.
  string(REGEX REPLACE "-frtti" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
endif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")

option(BERRYDB_BUILD_TESTS "Build BerryDB's unit tests" ON)
option(BERRYDB_BUILD_CLI "Build CLI for demo" ON)
option(BERRYDB_BUILD_BENCHMARKS "Build BerryDB's benchmarks" ON)
option(BERRYDB_USE_GLOG "Build with Google Logging" ON)

include(CheckIncludeFiles)
include(CheckIncludeFileCXX)
check_include_file_cxx("string_view" BERRYDB_PLATFORM_HAVE_STD_STRING_VIEW)

if(BERRYDB_USE_GLOG)
  # glog requires this setting to avoid using dynamic_cast.
  set(DISABLE_RTTI ON CACHE BOOL "" FORCE)

  # glog's test targets trigger deprecation warnings, and compiling them burns
  # CPU cycles on the CI.
  set(BUILD_TESTING_SAVED "${BUILD_TESTING}")
  set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
  add_subdirectory("${PROJECT_SOURCE_DIR}/third_party/glog" EXCLUDE_FROM_ALL)
  set(BUILD_TESTING "${BUILD_TESTING_SAVED}" CACHE BOOL "" FORCE)

  # glog triggers deprecation warnings on OSX.
  # https://github.com/google/glog/issues/185
  include(CheckCXXCompilerFlag)
  check_cxx_compiler_flag(-Wno-deprecated BERRYDB_HAVE_NO_DEPRECATED)
  if(BERRYDB_HAVE_NO_DEPRECATED)
    set_property(TARGET glog APPEND PROPERTY COMPILE_OPTIONS -Wno-deprecated)
  endif(BERRYDB_HAVE_NO_DEPRECATED)

  # glog triggers sign comparison warnings on gcc.
  check_cxx_compiler_flag(-Wno-sign-compare BERRYDB_HAVE_NO_SIGN_COMPARE)
  if(BERRYDB_HAVE_NO_SIGN_COMPARE)
    set_property(TARGET glog APPEND PROPERTY COMPILE_OPTIONS -Wno-sign-compare)
  endif(BERRYDB_HAVE_NO_SIGN_COMPARE)

  # glog triggers unused parameter warnings on clang.
  check_cxx_compiler_flag(-Wno-unused-parameter
                          BERRYDB_HAVE_NO_UNUSED_PARAMETER)
  if(BERRYDB_HAVE_NO_UNUSED_PARAMETER)
    set_property(TARGET glog
                 APPEND PROPERTY COMPILE_OPTIONS -Wno-unused-parameter)
  endif(BERRYDB_HAVE_NO_UNUSED_PARAMETER)

  set(BERRYDB_PLATFORM_BUILT_WITH_GLOG 1)
endif(BERRYDB_USE_GLOG)

configure_file(
  "${PROJECT_SOURCE_DIR}/platform/berrydb/platform/config.h.in"
  "${PROJECT_BINARY_DIR}/platform/berrydb/platform/config.h"
)

add_library(berrydb "")
target_sources(berrydb
  PRIVATE
    "${PROJECT_SOURCE_DIR}/src/api/catalog.cc"
    "${PROJECT_SOURCE_DIR}/src/api/options.cc"
    "${PROJECT_SOURCE_DIR}/src/api/ostream_ops.cc"
    "${PROJECT_SOURCE_DIR}/src/api/pool.cc"
    "${PROJECT_SOURCE_DIR}/src/api/space.cc"
    "${PROJECT_SOURCE_DIR}/src/api/status.cc"
    "${PROJECT_SOURCE_DIR}/src/api/store.cc"
    "${PROJECT_SOURCE_DIR}/src/api/transaction.cc"
    "${PROJECT_SOURCE_DIR}/src/api/vfs.cc"
    "${PROJECT_SOURCE_DIR}/src/format/store_header.cc"
    "${PROJECT_SOURCE_DIR}/src/format/store_header.h"
    "${PROJECT_SOURCE_DIR}/src/page.cc"
    "${PROJECT_SOURCE_DIR}/src/page.h"
    "${PROJECT_SOURCE_DIR}/src/catalog_impl.cc"
    "${PROJECT_SOURCE_DIR}/src/catalog_impl.h"
    "${PROJECT_SOURCE_DIR}/src/free_page_list_format.cc"
    "${PROJECT_SOURCE_DIR}/src/free_page_list_format.h"
    "${PROJECT_SOURCE_DIR}/src/free_page_list.cc"
    "${PROJECT_SOURCE_DIR}/src/free_page_list.h"
    "${PROJECT_SOURCE_DIR}/src/free_page_manager.cc"
    "${PROJECT_SOURCE_DIR}/src/free_page_manager.h"
    "${PROJECT_SOURCE_DIR}/src/page_pool.cc"
    "${PROJECT_SOURCE_DIR}/src/page_pool.h"
    "${PROJECT_SOURCE_DIR}/src/pool_impl.cc"
    "${PROJECT_SOURCE_DIR}/src/pool_impl.h"
    "${PROJECT_SOURCE_DIR}/src/space_impl.cc"
    "${PROJECT_SOURCE_DIR}/src/space_impl.h"
    "${PROJECT_SOURCE_DIR}/src/store_impl.cc"
    "${PROJECT_SOURCE_DIR}/src/store_impl.h"
    "${PROJECT_SOURCE_DIR}/src/transaction_impl.cc"
    "${PROJECT_SOURCE_DIR}/src/transaction_impl.h"
    "${PROJECT_SOURCE_DIR}/src/util/linked_list.h"
    "${PROJECT_SOURCE_DIR}/src/util/platform_allocator.h"
    "${PROJECT_SOURCE_DIR}/src/util/platform_deleter.h"
    "${PROJECT_SOURCE_DIR}/src/util/unique_ptr.h"
    "${PROJECT_SOURCE_DIR}/src/vfs/libc_vfs.cc"
  PUBLIC
    "${PROJECT_BINARY_DIR}/platform/berrydb/platform/config.h"
    "${PROJECT_SOURCE_DIR}/platform/berrydb/platform.h"
    "${PROJECT_SOURCE_DIR}/platform/berrydb/platform/alloc.h"
    "${PROJECT_SOURCE_DIR}/platform/berrydb/platform/dcheck.h"
    "${PROJECT_SOURCE_DIR}/platform/berrydb/platform/endianness.h"
    "${PROJECT_SOURCE_DIR}/include/berrydb.h"
    "${PROJECT_SOURCE_DIR}/include/berrydb/catalog.h"
    "${PROJECT_SOURCE_DIR}/include/berrydb/options.h"
    "${PROJECT_SOURCE_DIR}/include/berrydb/ostream_ops.h"
    "${PROJECT_SOURCE_DIR}/include/berrydb/pool.h"
    "${PROJECT_SOURCE_DIR}/include/berrydb/space.h"
    "${PROJECT_SOURCE_DIR}/include/berrydb/status.h"
    "${PROJECT_SOURCE_DIR}/include/berrydb/store.h"
    "${PROJECT_SOURCE_DIR}/include/berrydb/string_view.h"
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
  "${PROJECT_SOURCE_DIR}/platform"
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
  add_subdirectory("${PROJECT_SOURCE_DIR}/third_party/googletest")

  add_executable(berrydb_tests "")
  target_sources(berrydb_tests
    PRIVATE
      "${PROJECT_SOURCE_DIR}/src/api/ostream_ops_unittest.cc"
      "${PROJECT_SOURCE_DIR}/src/api/pool_unittest.cc"
      "${PROJECT_SOURCE_DIR}/src/api/status_unittest.cc"
      "${PROJECT_SOURCE_DIR}/src/api/store_unittest.cc"
      "${PROJECT_SOURCE_DIR}/src/api/string_view_unittest.cc"
      "${PROJECT_BINARY_DIR}/src/api/version_unittest.cc"
      "${PROJECT_SOURCE_DIR}/src/embedder_tests/alloc_unittest.cc"
      "${PROJECT_SOURCE_DIR}/src/embedder_tests/endianness_unittest.cc"
      "${PROJECT_SOURCE_DIR}/src/embedder_tests/vfs_unittest.cc"
      "${PROJECT_SOURCE_DIR}/src/format/store_header_unittest.cc"
      "${PROJECT_SOURCE_DIR}/src/free_page_list_format_unittest.cc"
      "${PROJECT_SOURCE_DIR}/src/free_page_list_unittest.cc"
      "${PROJECT_SOURCE_DIR}/src/page_pool_unittest.cc"
      "${PROJECT_SOURCE_DIR}/src/page_unittest.cc"
      "${PROJECT_SOURCE_DIR}/src/store_impl_unittest.cc"
      "${PROJECT_SOURCE_DIR}/src/test/block_access_file_wrapper.cc"
      "${PROJECT_SOURCE_DIR}/src/test/block_access_file_wrapper.h"
      "${PROJECT_SOURCE_DIR}/src/test/file_deleter.cc"
      "${PROJECT_SOURCE_DIR}/src/test/file_deleter.h"
      "${PROJECT_SOURCE_DIR}/src/test/file_deleter_unittest.cc"
      "${PROJECT_SOURCE_DIR}/src/test/test_main.cc"
      "${PROJECT_SOURCE_DIR}/src/util/linked_list_unittest.cc"
      "${PROJECT_SOURCE_DIR}/src/util/platform_allocator_unittest.cc"
      "${PROJECT_SOURCE_DIR}/src/util/platform_deleter_unittest.cc"
      "${PROJECT_SOURCE_DIR}/src/util/unique_ptr_unittest.cc"
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
      "${PROJECT_SOURCE_DIR}/third_party/boost/assert/include"
      "${PROJECT_SOURCE_DIR}/third_party/boost/config/include"
      "${PROJECT_SOURCE_DIR}/third_party/boost/core/include"
      "${PROJECT_SOURCE_DIR}/third_party/boost/detail/include"
      "${PROJECT_SOURCE_DIR}/third_party/boost/iterator/include"
      "${PROJECT_SOURCE_DIR}/third_party/boost/mpl/include"
      "${PROJECT_SOURCE_DIR}/third_party/boost/preprocessor/include"
      "${PROJECT_SOURCE_DIR}/third_party/boost/static_assert/include"
      "${PROJECT_SOURCE_DIR}/third_party/boost/throw_exception/include"
      "${PROJECT_SOURCE_DIR}/third_party/boost/tokenizer/include"
      "${PROJECT_SOURCE_DIR}/third_party/boost/type_traits/include"
  )
  target_sources(berrydb_cli
    PRIVATE
      "${PROJECT_SOURCE_DIR}/src/cli/cli.cc"
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
      "${PROJECT_SOURCE_DIR}/src/bench/benchmark_main.cc"
      "${PROJECT_SOURCE_DIR}/src/bench/crc32c_benchmark.cc"
      "${PROJECT_SOURCE_DIR}/src/bench/snappy_benchmark.cc"
      "${PROJECT_SOURCE_DIR}/src/bench/vfs_benchmark.cc"
      "${PROJECT_SOURCE_DIR}/src/test/file_deleter.cc"
      "${PROJECT_SOURCE_DIR}/src/test/file_deleter.h"
  )
  target_link_libraries(berrydb_bench berrydb)

  # This project uses Google benchmark for benchmarking.
  set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
  set(BENCHMARK_ENABLE_EXCEPTIONS OFF CACHE BOOL "" FORCE)
  add_subdirectory("${PROJECT_SOURCE_DIR}/third_party/benchmark")
  target_link_libraries(berrydb_bench benchmark)

  # This project uses crc32c.
  set(CRC32C_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(CRC32C_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
  set(CRC32C_USE_GLOG OFF CACHE BOOL "" FORCE)
  add_subdirectory("${PROJECT_SOURCE_DIR}/third_party/crc32c")
  target_link_libraries(berrydb_bench crc32c)

  # This project uses snappy.
  set(SNAPPY_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  add_subdirectory("${PROJECT_SOURCE_DIR}/third_party/snappy")
  target_link_libraries(berrydb_bench snappy)

  # Snappy triggers sign comparison warnings on clang.
  check_cxx_compiler_flag(-Wno-sign-compare BERRYDB_HAVE_NO_SIGN_COMPARE)
  if(BERRYDB_HAVE_NO_SIGN_COMPARE)
    set_property(TARGET snappy
                 APPEND PROPERTY COMPILE_OPTIONS -Wno-sign-compare)
  endif(BERRYDB_HAVE_NO_SIGN_COMPARE)

  # Snappy triggers unused parameter warnings on clang.
  check_cxx_compiler_flag(-Wno-unused-parameter
                          BERRYDB_HAVE_NO_UNUSED_PARAMETER)
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

  if(BERRYDB_USE_GLOG)
    target_link_libraries(berrydb_bench glog)
  endif(BERRYDB_USE_GLOG)

  # Warnings as errors in Visual Studio for this project's targets.
  if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set_property(TARGET berrydb_bench APPEND PROPERTY COMPILE_OPTIONS "/WX")
  endif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
endif(BERRYDB_BUILD_BENCHMARKS)
