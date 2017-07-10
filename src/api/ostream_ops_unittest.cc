// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "berrydb/ostream_ops.h"

#include <string>
#include <sstream>
#include <unordered_set>

#include "gtest/gtest.h"

#include "berrydb/status.h"

namespace berrydb {

TEST(OstreamOpsTest, StatusOutput) {
  // Check that each status results in a different string.
  std::unordered_set<std::string> outputs;

  std::string ending(" END_CHECK");

  for (size_t i = static_cast<size_t>(Status::kSuccess);
      i < static_cast<size_t>(Status::kFirstInvalidValue); ++i) {
    Status status = static_cast<Status>(i);

    std::stringstream sstream;
    sstream << status << ending;
    std::string output = sstream.str();

    EXPECT_GT(output.size(), ending.size());
    EXPECT_EQ(ending, output.substr(output.size() - ending.size()));

    EXPECT_TRUE(outputs.count(output) == 0);
    outputs.insert(output);
  }
}

}  // namespace berrydb
