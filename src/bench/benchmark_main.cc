// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "berrydb/platform.h"

#include "benchmark/benchmark.h"

int main(int argc, char** argv) {
#ifdef BERRYDB_PLATFORM_BUILT_WITH_GLOG
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();
#endif  // BERRYDB_PLATFORM_BUILT_WITH_GLOG

  benchmark::Initialize(&argc, argv);
  benchmark::RunSpecifiedBenchmarks();
  return 0;
}
