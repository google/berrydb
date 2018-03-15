# BerryDB - An Experimental Key-Value Store

[![Build Status](https://travis-ci.org/google/berrydb.svg?branch=master)](https://travis-ci.org/google/berrydb)
[![Build Status](https://ci.appveyor.com/api/projects/status/rcys8rqbjpymauuu/branch/master?svg=true)](https://ci.appveyor.com/project/pwnall/berrydb)

**This is not an official Google product.**
[LevelDB](https://github.com/google/leveldb) is a stable key-value store that is
widely used at Google.

This is an experimental implementation of a key-value store.


## Prerequisites

This project uses [CMake](https://cmake.org/) for building and testing. CMake is
available in all popular Linux distributions, as well as in
[Homebrew](https://brew.sh/).

This project uses submodules for dependency management.

```bash
git submodule update --init --recursive
```

If you're using [Atom](https://atom.io/), the following packages can help.

```bash
apm install autocomplete-clang build build-cmake clang-format \
    clang-tools-extra docblockr language-cmake linter linter-clang
```

If you don't mind more setup in return for more speed, replace
`autocomplete-clang` and `linter-clang` with `you-complete-me`. This requires
[setting up ycmd](https://github.com/Valloric/ycmd#building).

```bash
apm install autocomplete-plus build build-cmake clang-format \
    clang-tools-extra docblockr language-cmake linter you-complete-me
```


## Building

The following commands build the project.

```bash
mkdir build
cd build
cmake .. && cmake --build .
```


## Development

The following command (when executed from `build/`) (re)builds the project and
runs the tests.

```bash
cmake .. && cmake --build . && ctest --output-on-failure
```

### Android Testing

The following command builds the project against the Android NDK, which is
useful for benchmarking against ARM processors.

```bash
cmake .. -DCMAKE_SYSTEM_NAME=Android -DCMAKE_ANDROID_ARCH_ABI=arm64-v8a \
    -DCMAKE_ANDROID_NDK=$HOME/Library/Android/sdk/ndk-bundle \
    -DCMAKE_ANDROID_NDK_TOOLCHAIN_VERSION=clang \
    -DCMAKE_ANDROID_STL_TYPE=c++_static -DBERRYDB_USE_GLOG=0 \
    -DCMAKE_BUILD_TYPE=Release && cmake --build .
```

The following commands install and run the benchmarks.

```bash
adb push berrydb_bench /data/local/tmp
adb shell chmod +x /data/local/tmp/berrydb_bench
adb shell 'cd /data/local/tmp && ./berrydb_bench'
adb shell rm /data/local/tmp/berrydb_bench
```

The following commands install and run the tests.

```bash
adb push berrydb_tests /data/local/tmp
adb shell chmod +x /data/local/tmp/berrydb_tests
adb shell 'cd /data/local/tmp && ./berrydb_tests'
adb shell rm /data/local/tmp/berrydb_tests
```

Most third-party libraries used by this project can only be used in specific
components. `CMakeLists.txt` enforces these constraints, and
[third_party/README.md](./third_party/README.md) describes the motivations
behind them.

### Static Analysis

This project is experimenting with
[Facebook Infer](https://facebook.github.io/infer) for static analysis. The
following command collects diagnostics.

```bash
infer compile -- cmake -DBERRYDB_USE_GLOG=0 -DBERRYDB_BUILD_BENCHMARKS=0 \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo .. && infer run -- cmake --build .
```
