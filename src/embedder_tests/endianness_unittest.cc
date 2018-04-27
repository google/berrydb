// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "berrydb/platform.h"

#include "berrydb/span.h"
#include "../util/span_util.h"

#include "gtest/gtest.h"

namespace berrydb {

TEST(EndiannessTest, LoadMatchesStore) {
  alignas(8) uint8_t buffer_bytes[32];
  span<uint8_t> buffer(buffer_bytes);
  FillSpan(make_span(buffer_bytes), 0xCD);

  uint64_t magic1 = 0x4265727279444220;
  uint64_t magic2 = 0x444253746f726520;

  StoreUint64(magic1, buffer.subspan(8, 8));
  for (size_t i = 0; i < 8; ++i)
    EXPECT_EQ(0xCD, buffer_bytes[i]);
  for (size_t i = 16; i < 32; ++i)
    EXPECT_EQ(0xCD, buffer_bytes[i]);

  EXPECT_EQ(magic1, LoadUint64(buffer.subspan(8, 8)));

  StoreUint64(magic2, buffer.subspan(8, 8));
  EXPECT_EQ(magic2, LoadUint64(buffer.subspan(8, 8)));

  StoreUint64(magic1, buffer.subspan(16, 8));
  EXPECT_EQ(magic2, LoadUint64(buffer.subspan(8, 8)));
  EXPECT_EQ(magic1, LoadUint64(buffer.subspan(16, 8)));

  EXPECT_EQ(0xCDCDCDCDCDCDCDCDU, LoadUint64(buffer.subspan(0, 8)));
  EXPECT_EQ(0xCDCDCDCDCDCDCDCDU, LoadUint64(buffer.subspan(24, 8)));
}

}  // namespace berrydb
