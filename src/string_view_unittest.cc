// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "berrydb/platform.h"

#include "gtest/gtest.h"

namespace berrydb {

TEST(StringViewTest, ConstructorEmpty) {
  string_view sv;

  EXPECT_EQ(sv.data(), nullptr);
  EXPECT_EQ(sv.length(), 0);
  EXPECT_EQ(sv.size(), 0);
  EXPECT_EQ(sv.empty(), true);
}

TEST(StringViewTest, ConstructorCString) {
  const char cstring[] = "hello world";
  string_view sv(cstring);

  EXPECT_EQ(sv.data(), cstring);
  EXPECT_EQ(sv.length(), 11);
  EXPECT_EQ(sv.size(), 11);
  EXPECT_EQ(sv.empty(), false);
}

TEST(StringViewTest, ConstructorDataSize) {
  const char* data = reinterpret_cast<const char*>(0xBEEFBEEF);
  string_view sv(data, 42);

  EXPECT_EQ(sv.data(), data);
  EXPECT_EQ(sv.length(), 42);
  EXPECT_EQ(sv.size(), 42);
  EXPECT_EQ(sv.empty(), false);
}

TEST(StringViewTest, ConstructorCopy) {
  const char* data = reinterpret_cast<const char*>(0xBEEFBEEF);
  string_view source_sv(data, 42);
  string_view sv(source_sv);

  EXPECT_EQ(sv.data(), data);
  EXPECT_EQ(sv.length(), 42);
  EXPECT_EQ(sv.size(), 42);
  EXPECT_EQ(sv.empty(), false);
}

TEST(StringViewTest, ElementAccessor) {
  const char buffer[] = "hello world";
  string_view sv(buffer, 11);

  EXPECT_EQ(sv[0], 'h');
  EXPECT_EQ(sv[1], 'e');
  EXPECT_EQ(sv[5], ' ');
  EXPECT_EQ(sv[9], 'l');
  EXPECT_EQ(sv[10], 'd');
}

TEST(StringViewTest, RemovePrefixNone) {
  const char buffer[] = "hello world";
  string_view sv(buffer, 11);

  sv.remove_prefix(0);
  EXPECT_EQ(sv.data(), buffer);
  EXPECT_EQ(sv.length(), 11);
  EXPECT_EQ(sv.size(), 11);
  EXPECT_EQ(sv.empty(), false);
}

TEST(StringViewTest, RemovePrefixSingleElement) {
  const char buffer[] = "hello world";
  string_view sv(buffer, 11);

  sv.remove_prefix(1);
  EXPECT_EQ(sv.data(), buffer + 1);
  EXPECT_EQ(sv.length(), 10);
  EXPECT_EQ(sv.size(), 10);
  EXPECT_EQ(sv.empty(), false);
}

TEST(StringViewTest, RemovePrefixMultiElement) {
  const char buffer[] = "hello world";
  string_view sv(buffer, 11);

  sv.remove_prefix(6);
  EXPECT_EQ(sv.data(), buffer + 6);
  EXPECT_EQ(sv.length(), 5);
  EXPECT_EQ(sv.size(), 5);
  EXPECT_EQ(sv.empty(), false);
}

TEST(StringViewTest, RemovePrefixWhole) {
  const char buffer[] = "hello world";
  string_view sv(buffer, 11);

  sv.remove_prefix(11);
  EXPECT_EQ(sv.data(), buffer + 11);
  EXPECT_EQ(sv.length(), 0);
  EXPECT_EQ(sv.size(), 0);
  EXPECT_EQ(sv.empty(), true);
}

TEST(StringViewTest, RemoveSuffixNone) {
  const char buffer[] = "hello world";
  string_view sv(buffer, 11);

  sv.remove_suffix(0);
  EXPECT_EQ(sv.data(), buffer);
  EXPECT_EQ(sv.length(), 11);
  EXPECT_EQ(sv.size(), 11);
  EXPECT_EQ(sv.empty(), false);
}

TEST(StringViewTest, RemoveSuffixPartial) {
  const char buffer[] = "hello world";
  string_view sv(buffer, 11);

  sv.remove_suffix(1);
  EXPECT_EQ(sv.data(), buffer);
  EXPECT_EQ(sv.length(), 10);
  EXPECT_EQ(sv.size(), 10);
  EXPECT_EQ(sv.empty(), false);
}

TEST(StringViewTest, RemoveSuffixWhole) {
  const char buffer[] = "hello world";
  string_view sv(buffer, 11);

  sv.remove_suffix(11);
  EXPECT_EQ(sv.data(), buffer);
  EXPECT_EQ(sv.length(), 0);
  EXPECT_EQ(sv.size(), 0);
  EXPECT_EQ(sv.empty(), true);
}

TEST(StringViewTest, SubstringDefaultCount) {
  const char buffer[] = "hello world";
  string_view sv(buffer, 11);

  sv = sv.substr(6);
  EXPECT_EQ(sv.data(), buffer + 6);
  EXPECT_EQ(sv.length(), 5);
  EXPECT_EQ(sv.size(), 5);
  EXPECT_EQ(sv.empty(), false);
}

TEST(StringViewTest, SubstringZeroCount) {
  const char buffer[] = "hello world";
  string_view sv(buffer, 11);

  sv = sv.substr(11, 0);
  EXPECT_EQ(sv.data(), buffer + 11);
  EXPECT_EQ(sv.length(), 0);
  EXPECT_EQ(sv.size(), 0);
  EXPECT_EQ(sv.empty(), true);
}

TEST(StringViewTest, SubstringCountWithinSize) {
  const char buffer[] = "hello world";
  string_view sv(buffer, 11);

  sv = sv.substr(6, 3);
  EXPECT_EQ(sv.data(), buffer + 6);
  EXPECT_EQ(sv.length(), 3);
  EXPECT_EQ(sv.size(), 3);
  EXPECT_EQ(sv.empty(), false);
}

TEST(StringViewTest, SubstringCountExceedsSize) {
  const char buffer[] = "hello world";
  string_view sv(buffer, 11);

  sv = sv.substr(6, 100);
  EXPECT_EQ(sv.data(), buffer + 6);
  EXPECT_EQ(sv.length(), 5);
  EXPECT_EQ(sv.size(), 5);
  EXPECT_EQ(sv.empty(), false);
}

TEST(StringViewTest, SubstringStartIndexEqualsSize) {
  const char buffer[] = "hello world";
  string_view sv(buffer, 11);

  sv = sv.substr(11, 100);
  EXPECT_EQ(sv.data(), buffer + 11);
  EXPECT_EQ(sv.length(), 0);
  EXPECT_EQ(sv.size(), 0);
  EXPECT_EQ(sv.empty(), true);
}

TEST(StringViewTest, CompareEqualTo) {
  const char buffer[] = "hello world";
  string_view sv1(buffer, 11);
  string_view sv2(buffer, 11);

  EXPECT_EQ(sv1.compare(sv2), 0);
  EXPECT_EQ(sv2.compare(sv1), 0);
}

TEST(StringViewTest, CompareDifferentLengths) {
  const char buffer[] = "hello world";
  string_view sv1(buffer, 5);
  string_view sv2(buffer, 10);

  EXPECT_LT(sv1.compare(sv2), 0);
  EXPECT_GT(sv2.compare(sv1), 0);
}

TEST(StringViewTest, CompareDifferentValues) {
  const char buffer1[] = "a";
  string_view sv1(buffer1, 1);
  const char buffer2[] = "b";
  string_view sv2(buffer2, 1);

  EXPECT_LT(sv1.compare(sv2), 0);
  EXPECT_GT(sv2.compare(sv1), 0);
}

TEST(StringViewTest, CompareIntermediateValues) {
  const char buffer1[] = "aaaa";
  string_view sv1(buffer1, 4);
  const char buffer2[] = "aaba";
  string_view sv2(buffer2, 4);

  EXPECT_LT(sv1.compare(sv2), 0);
  EXPECT_GT(sv2.compare(sv1), 0);
}

TEST(StringViewTest, CompareTerminatingValues) {
  const char buffer1[] = "aaaa";
  string_view sv1(buffer1, 4);
  const char buffer2[] = "aaab";
  string_view sv2(buffer2, 4);

  EXPECT_LT(sv1.compare(sv2), 0);
  EXPECT_GT(sv2.compare(sv1), 0);
}

TEST(StringViewTest, CompareWeightsValueOverLength) {
  const char buffer1[] = "b";
  string_view sv1(buffer1, 1);
  const char buffer2[] = "a\0\0\0";
  string_view sv2(buffer2, 1);

  EXPECT_GT(sv1.compare(sv2), 0);
}

TEST(StringViewTest, InequalityOperatorsForSelf) {
  const char buffer[] = "hello world";
  string_view sv(buffer, 11);

  EXPECT_EQ(sv, sv);
  EXPECT_EQ(sv != sv, false);
  EXPECT_GE(sv, sv);
  EXPECT_EQ(sv > sv, false);
  EXPECT_LE(sv, sv);
  EXPECT_EQ(sv < sv, false);
}

TEST(StringViewTest, InequalityOperatorsForEqualObjects) {
  const char buffer[] = "hello world";
  string_view sv1(buffer, 11);
  string_view sv2(buffer, 11);

  EXPECT_EQ(sv1, sv2);
  EXPECT_EQ(sv2, sv1);
  EXPECT_EQ(sv1 != sv2, false);
  EXPECT_EQ(sv2 != sv1, false);
  EXPECT_GE(sv1, sv2);
  EXPECT_GE(sv2, sv1);
  EXPECT_EQ(sv1 > sv2, false);
  EXPECT_EQ(sv2 > sv1, false);
  EXPECT_LE(sv1, sv2);
  EXPECT_LE(sv2, sv1);
  EXPECT_EQ(sv1 < sv2, false);
  EXPECT_EQ(sv2 < sv1, false);
}

TEST(StringViewTest, InequalityOperatorsForDifferentObjects) {
  const char buffer1[] = "hello world";
  string_view sv1(buffer1, 11);
  const char buffer2[] = "byebye world";
  string_view sv2(buffer2, 11);

  EXPECT_NE(sv1, sv2);
  EXPECT_NE(sv2, sv1);
  EXPECT_EQ(sv1 == sv2, false);
  EXPECT_EQ(sv2 == sv1, false);
  EXPECT_GE(sv1, sv2);
  EXPECT_EQ(sv2 >= sv1, false);
  EXPECT_GT(sv1, sv2);
  EXPECT_EQ(sv2 > sv1, false);
  EXPECT_LE(sv2, sv1);
  EXPECT_EQ(sv1 <= sv2, false);
  EXPECT_LT(sv2, sv1);
  EXPECT_EQ(sv1 < sv2, false);
}

}  // namespace berrydb
