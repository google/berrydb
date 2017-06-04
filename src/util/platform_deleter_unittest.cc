// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "./platform_deleter.h"

#include <type_traits>

#include "gtest/gtest.h"

#include "berrydb/platform.h"

namespace berrydb {

struct TestDeleterReleasableType {
  TestDeleterReleasableType() noexcept { release_calls = 0; }
  void Release() noexcept { ++release_calls; }
  size_t release_calls;
};

static_assert(std::is_empty<PlatformDeleter<TestDeleterReleasableType>>::value,
    "PlatformDeleter should not have state");

TEST(PlatformDeleterTest, CallsRelease) {
  TestDeleterReleasableType releasable;
  ASSERT_EQ(0U, releasable.release_calls);

  PlatformDeleter<TestDeleterReleasableType> deleter;
  deleter(&releasable);
  ASSERT_EQ(1U, releasable.release_calls);
}

}  // namespace berrydb
