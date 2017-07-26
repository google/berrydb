# BerryDB Third-Party libraries

The command-line tool code in `cli/` uses
[the Boost C++ libraries](http://www.boost.org/). Boost is a large unwieldy
library, so imposing it on the project's consumers is highly undesirable.
Therefore, Boost must not be used in the core library. Boost is currently
prohibited in tests (outside `cli/`), in order to make sure that the core
library's APIs are usable by consumers who do not include Boost.

The testing code uses [googletest](https://github.com/google/googletest).
Outside testing, the project's the public headers must not use `gtest_prod.h`.
The internal headers are currently prohibited from using `gtest_prod.h` as well,
because having to friend test classes is (arguably) a code smell.

The core library can optionally use the [glog](https://github.com/google/glog)
implementation of the `DCHECK` macro family via the `berrydb/platform/dcheck.h`
header. The core library must compile on all supported platforms without `glog`,
in which case `berrydb/platform/dcheck.h` supplies its own `DCHECK`
implementation, which is based on C++'s `assert`. When `glog` is available,
testing code uses its stack trace-rendering signal handler, via
`test/test_main.cc`.

Asides from the usage described above, `glog` may not be currently used directly
by any component in the project. For example, the logging macros
(`LOG(DEBUG) << "hi";`) can be very useful for debugging, but must not be
checked into the project's repository.

The benchmarking code uses
[Google benchmark](https://github.com/google/benchmark). Code outside the
`src/bench/` directory may not reach into Google benchmark.
