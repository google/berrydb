// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "./free_page_list_format.h"

#include "berrydb/span.h"
#include "berrydb/types.h"
#include "./util/span_util.h"

#include "gtest/gtest.h"

namespace berrydb {

TEST(FreePageListFormatTest, NextEntryOffset) {
  alignas(8) uint8_t page_data_bytes[256];
  span<uint8_t> page_data(page_data_bytes);
  FillSpan(page_data, 0xCC);

  size_t next_entry_offset = 0x12345678;
  FreePageListFormat::SetNextEntryOffset(next_entry_offset, page_data);
  EXPECT_EQ(next_entry_offset, FreePageListFormat::NextEntryOffset(page_data));

  size_t bytes_changed = 0;
  for (size_t i = 0; i < 256; ++i) {
    if (page_data[i] != 0xCC)
      bytes_changed += 1;
  }
  EXPECT_EQ(8U, bytes_changed);
}

TEST(FreePageListFormatTest, NextPageId64) {
  alignas(8) uint8_t page_data_bytes[256];
  span<uint8_t> page_data(page_data_bytes);
  FillSpan(page_data, 0xCC);

  uint64_t next_page_id64 = 0x1234567890ABCDEF;
  FreePageListFormat::SetNextPageId64(next_page_id64, page_data);
  EXPECT_EQ(next_page_id64, FreePageListFormat::NextPageId64(page_data));

  size_t bytes_changed = 0;
  for (size_t i = 0; i < 256; ++i) {
    if (page_data[i] != 0xCC)
      bytes_changed += 1;
  }
  EXPECT_EQ(8U, bytes_changed);
}

TEST(FreePageListFormatTest, PageFields) {
  alignas(8) uint8_t page_data_bytes[256];
  span<uint8_t> page_data(page_data_bytes);
  FillSpan(page_data, 0xCC);

  size_t next_entry_offset = 0x12345678;
  uint64_t next_page_id64 = 0x1234567890ABCDEF;

  FreePageListFormat::SetNextEntryOffset(next_entry_offset, page_data);
  FreePageListFormat::SetNextPageId64(next_page_id64, page_data);

  EXPECT_EQ(next_entry_offset, FreePageListFormat::NextEntryOffset(page_data));
  EXPECT_EQ(next_page_id64, FreePageListFormat::NextPageId64(page_data));

  size_t bytes_changed = 0;
  for (size_t i = 0; i < 256; ++i) {
    if (page_data[i] != 0xCC)
      bytes_changed += 1;
  }
  EXPECT_EQ(16U, bytes_changed);
}

TEST(FreePageListFormatTest, IsCorruptEntryOffset) {
  EXPECT_FALSE(FreePageListFormat::IsCorruptEntryOffset(16, 256));
  for (size_t i = 17; i < 24; ++i)
    EXPECT_TRUE(FreePageListFormat::IsCorruptEntryOffset(i, 256));
  EXPECT_FALSE(FreePageListFormat::IsCorruptEntryOffset(24, 256));
  for (size_t i = 25; i < 32; ++i)
    EXPECT_TRUE(FreePageListFormat::IsCorruptEntryOffset(i, 256));
  EXPECT_FALSE(FreePageListFormat::IsCorruptEntryOffset(32, 256));

  EXPECT_FALSE(FreePageListFormat::IsCorruptEntryOffset(248, 256));
  EXPECT_TRUE(FreePageListFormat::IsCorruptEntryOffset(256, 256));

  EXPECT_TRUE(FreePageListFormat::IsCorruptEntryOffset(1024, 256));
  EXPECT_FALSE(FreePageListFormat::IsCorruptEntryOffset(256, 1024));
  EXPECT_TRUE(FreePageListFormat::IsCorruptEntryOffset(1024, 1024));
}

}  // namespace berrydb
