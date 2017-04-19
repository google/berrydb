# BerryDB

[![Build Status](https://travis-ci.org/google/berrydb.svg?branch=master)](https://travis-ci.org/google/berrydb)
[![Build Status](https://ci.appveyor.com/api/projects/status/399bd926yvoe67o5/branch/master?svg=true)](https://travis-ci.org/pwnall/berrydb)

## Prerequisites

This project uses [CMake](https://cmake.org/) for building and testing. CMake is
available in all popular Linux distributions, as well as in
[Homebrew](https://brew.sh/).

This project uses submodules for dependency management.

```bash
git submodules update --init --recursive
```

If you're using [Atom](https://atom.io/), the following packages can help.

```bash
apm install autocomplete-clang build build-cmake clang-format docblockr \
    language-cmake linter linter-clang
```

## Building

The following commands build the project.

```bash
mkdir out
cd out
cmake .. && cmake --build .
```

## Development

The following command (when executed from `out/`) (re)builds the project and
runs the tests.

```bash
cmake .. && cmake --build . && ctest --output-on-failure
```
