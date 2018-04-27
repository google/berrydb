// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "berrydb/platform.h"

#include "gtest/gtest.h"

#include "berrydb/span.h"
#include "../util/span_util.h"

namespace berrydb {

static_assert((sizeof(size_t) & (sizeof(size_t) - 1)) == 0,
    "Allocate and Deallocate assume that sizeof(size_t) is a power of two");

TEST(AllocTest, DoesNotCrash) {
  void* buffer_bytes = Allocate(64);

  span<uint8_t> buffer(static_cast<uint8_t*>(buffer_bytes), 64);
  FillSpan(buffer, 0);

  Deallocate(buffer_bytes, 64);
}

}  // namespace berrydb
