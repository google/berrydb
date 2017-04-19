// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "berrydb/platform.h"

#include "gtest/gtest.h"

namespace berrydb {

TEST(StringViewTest, Constructor_Empty) {
  string_view sv;

  ASSERT_EQ(sv.data(), nullptr);
  ASSERT_EQ(sv.length(), 0);
  ASSERT_EQ(sv.size(), 0);
  ASSERT_EQ(sv.empty(), true);
}

TEST(StringViewTest, Constructor_CString) {
  const char cstring[] = "hello world";
  string_view sv(cstring);

  ASSERT_EQ(sv.data(), cstring);
  ASSERT_EQ(sv.length(), 11);
  ASSERT_EQ(sv.size(), 11);
  ASSERT_EQ(sv.empty(), false);
}

TEST(StringViewTest, Constructor_Data_Size) {
  const char* data = reinterpret_cast<const char*>(0xBEEFBEEF);
  string_view sv(data, 42);

  ASSERT_EQ(sv.data(), data);
  ASSERT_EQ(sv.length(), 42);
  ASSERT_EQ(sv.size(), 42);
  ASSERT_EQ(sv.empty(), false);
}

}  // namespace berrydb
