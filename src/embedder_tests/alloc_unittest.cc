// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "berrydb/platform.h"

#include <cstring>

#include "gtest/gtest.h"

namespace berrydb {

static_assert((sizeof(size_t) & (sizeof(size_t) - 1)) == 0,
    "Allocate and Deallocate assume that sizeof(size_t) is a power of two");

TEST(AllocTest, DoesNotCrash) {
  void* buffer = Allocate(64);
  std::memset(buffer, '\0', 64);
  Deallocate(buffer, 64);
}

}  // namespace berrydb
