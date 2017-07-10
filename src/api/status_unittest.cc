// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "berrydb/status.h"

#include <cstring>
#include <unordered_set>

#include "gtest/gtest.h"

namespace berrydb {

TEST(StatusTest, StatusToCString) {
  // Check that each status results in a different string.
  std::unordered_set<const char *> values;

  for (size_t i = static_cast<size_t>(Status::kSuccess);
      i < static_cast<size_t>(Status::kFirstInvalidValue); ++i) {
    Status status = static_cast<Status>(i);
    const char* c_string = StatusToCString(status);

    EXPECT_TRUE(std::strlen(c_string) > 0);
    EXPECT_TRUE(values.count(c_string) == 0);
    values.insert(c_string);
  }
}

}  // namespace berrydb
