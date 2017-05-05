// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "berrydb/platform.h"

#include "gtest/gtest.h"

int main(int argc, char **argv) {
#ifdef BERRYDB_PLATFORM_BUILT_WITH_GLOG
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();
#endif  // BERRYDB_PLATFORM_BUILT_WITH_GLOG
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
