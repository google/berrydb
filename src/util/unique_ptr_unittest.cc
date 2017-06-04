// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "./unique_ptr.h"

#include <memory>

#include "gtest/gtest.h"

namespace berrydb {

struct TestUniquePtrReleasableType {
  TestUniquePtrReleasableType() noexcept { release_calls = 0; }
  void Release() noexcept { ++release_calls; }
  size_t release_calls;
};

static_assert(
    sizeof(UniquePtr<TestUniquePtrReleasableType>) ==
    sizeof(std::unique_ptr<TestUniquePtrReleasableType>),
    "UniquePtr should not have any memory overhead over std::unique_ptr");

TEST(UniquePtrTest, CallsRelease) {
  TestUniquePtrReleasableType releasable;
  ASSERT_EQ(0U, releasable.release_calls);

  {
    UniquePtr<TestUniquePtrReleasableType> ptr(&releasable);
    EXPECT_EQ(0U, releasable.release_calls);
  }
  EXPECT_EQ(1U, releasable.release_calls);

  {
    UniquePtr<TestUniquePtrReleasableType> ptr(&releasable);
    EXPECT_EQ(1U, releasable.release_calls);
  }
  EXPECT_EQ(2U, releasable.release_calls);
}

}  // namespace berrydb
