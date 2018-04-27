// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "./store_header.h"

#include "berrydb/span.h"
#include "berrydb/types.h"
#include "../util/span_util.h"

#include "gtest/gtest.h"

namespace berrydb {

TEST(StoreHeaderTest, SerializeDeserialize) {
  alignas(8) uint8_t buffer_bytes[2 * StoreHeader::kSerializedSize];
  span<uint8_t> buffer(buffer_bytes);
  FillSpan(buffer, 0xCD);

  StoreHeader header;
  header.page_shift = 12;
  header.page_count = 0xc0decdef;
  header.free_list_head_page = 0x12345678;
  header.Serialize(buffer);

  for (size_t i = StoreHeader::kSerializedSize; i < buffer.size(); ++i)
    EXPECT_EQ(0xCD, buffer[i]);

  StoreHeader header2;
  EXPECT_EQ(true, header2.Deserialize(buffer));
  EXPECT_EQ(header.page_shift, header2.page_shift);
  EXPECT_EQ(header.page_count, header2.page_count);
  EXPECT_EQ(header.free_list_head_page, header2.free_list_head_page);
}

TEST(StoreHeaderTest, HeaderErrors) {
  alignas(8) uint8_t buffer_bytes[StoreHeader::kSerializedSize];
  span<uint8_t> buffer(buffer_bytes);
  StoreHeader header;
  header.page_shift = 12;
  header.free_list_head_page = 0x12345678;
  header.page_count = 0xc0decdef;
  header.Serialize(buffer);

  StoreHeader header2;
  ASSERT_EQ(true, header2.Deserialize(buffer));

  // The first 24 bytes (including the version number) are effectively a fixed
  // header. Any change there should result in a de-serialization error.
  for (size_t i = 0; i < 24; ++i) {
    for (size_t j = 0; j < 8; ++j) {
      uint8_t mask = 1 << j;
      buffer[i] ^= mask;
      EXPECT_FALSE(header2.Deserialize(buffer));
      buffer[i] ^= mask;
      ASSERT_EQ(true, header2.Deserialize(buffer));
    }
  }
}

}  // namespace berrydb
