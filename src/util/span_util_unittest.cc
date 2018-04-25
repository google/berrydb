// Copyright 2018 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "./span_util.h"

#include <cstdint>

#include "gtest/gtest.h"

namespace berrydb {

TEST(SpanUtil, FillSpan) {
  uint16_t data[5];

  FillSpan(make_span(data), 0x1234);
  for (size_t i = 0; i < 5; ++i)
    EXPECT_EQ(0x1234U, data[i]);

  FillSpan(make_span(data).first(0), 0x5678);
  for (size_t i = 0; i < 5; ++i)
    EXPECT_EQ(0x1234U, data[i]);

  FillSpan(make_span(data).subspan(1, 3), 0x5678);
  EXPECT_EQ(0x1234U, data[0]);
  EXPECT_EQ(0x5678U, data[1]);
  EXPECT_EQ(0x5678U, data[2]);
  EXPECT_EQ(0x5678U, data[3]);
  EXPECT_EQ(0x1234U, data[4]);
}

TEST(SpanUtil, CopySpan) {
  const uint16_t from[4] = {0x1234, 0x5678, 0x9abc, 0xdef0};
  uint16_t to[5] = {0xcdcd, 0xcdcd, 0xcdcd, 0xcdcd, 0xcdcd};

  {
    uint16_t golden[5] = {0x1234, 0x5678, 0x9abc, 0xdef0, 0xcdcd};
    CopySpan(make_span(from), make_span(to));
    EXPECT_EQ(make_span(to), make_span(golden));
  }

  uint16_t other[4] = {0xfefe, 0xfefe, 0xfefe, 0xfefe};
  {
    uint16_t golden[5] = {0x1234, 0x5678, 0x9abc, 0xdef0, 0xcdcd};
    CopySpan(make_span(other).first(0), make_span(to));
    EXPECT_EQ(make_span(to), make_span(golden));
  }

  {
    uint16_t golden[5] = {0x1234, 0x5678, 0xfefe, 0xdef0, 0xcdcd};
    CopySpan(make_span(other).last(1), make_span(to).subspan(2, 2));
    EXPECT_EQ(make_span(to), make_span(golden));
  }
}

}  // namespace berrydb
