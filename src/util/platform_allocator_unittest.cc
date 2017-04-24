// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "./platform_allocator.h"

#include <cstring>

#include "gtest/gtest.h"

namespace berrydb {

static_assert(std::is_empty<PlatformAllocator<int>>::value,
    "PlatformAllocator should not have state");

TEST(PlatformAllocatorTest, DoesNotCrash) {
  PlatformAllocator<size_t> allocator;

  size_t* buffer = allocator.allocate(16);
  std::memset(buffer, '\0', sizeof(size_t) * 16);
  allocator.deallocate(buffer, 16);
}

}  // namespace berrydb
