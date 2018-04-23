// Copyright 2018 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "berrydb/span.h"

#include <type_traits>

#include "berrydb/types.h"

#include "gtest/gtest.h"

namespace berrydb {

namespace {

// C++17 is_same_v. inline in C++17.
template<typename T, typename U>
constexpr bool is_same_v = std::is_same<T, U>::value;

}  // anonymous namespace

static_assert(
    is_same_v<uint32_t, berrydb::span<uint32_t>::element_type>,
    "span::element_type correctness");
static_assert(
    is_same_v<const uint32_t, berrydb::span<const uint32_t>::element_type>,
    "span::element_type correctness");

static_assert(
    is_same_v<uint32_t, berrydb::span<uint32_t>::value_type>,
    "span::value_type correctness");
static_assert(
    is_same_v<uint32_t, berrydb::span<const uint32_t>::value_type>,
    "span::value_type correctness");

static_assert(
    is_same_v<uint32_t*, berrydb::span<uint32_t>::pointer>,
    "span::pointer type correctness");
static_assert(
    is_same_v<const uint32_t*, berrydb::span<const uint32_t>::pointer>,
    "span::pointer type correctness");

static_assert(
    is_same_v<uint32_t&, berrydb::span<uint32_t>::reference>,
    "span::reference type correctness");
static_assert(
    is_same_v<const uint32_t&, berrydb::span<const uint32_t>::reference>,
    "span::reference type correctness");

static_assert(
    is_same_v<
        std::reverse_iterator<berrydb::span<uint32_t>::iterator>,
        berrydb::span<uint32_t>::reverse_iterator>,
    "span::reverse_iterator type correctness");
static_assert(
    is_same_v<
        std::reverse_iterator<berrydb::span<const uint32_t>::iterator>,
        berrydb::span<const uint32_t>::reverse_iterator>,
    "span::reverse_iterator type correctness");

static_assert(
    is_same_v<
        std::reverse_iterator<berrydb::span<uint32_t>::iterator>,
        berrydb::span<uint32_t>::reverse_iterator>,
    "span::reverse_iterator type correctness");
static_assert(
    is_same_v<
        std::reverse_iterator<berrydb::span<const uint32_t>::iterator>,
        berrydb::span<const uint32_t>::reverse_iterator>,
    "span::reverse_iterator type correctness");

TEST(SpanTest, ConstructorEmpty) {
  berrydb::span<uint8_t> span;

  static_assert(
      is_same_v<uint8_t*, decltype(span.data())>,
      "data() type correctness");

  EXPECT_EQ(nullptr, span.data());
  EXPECT_EQ(0U, span.size());
  EXPECT_EQ(0U, span.size_bytes());
  EXPECT_TRUE(span.empty());

  EXPECT_EQ(span.begin(), span.end());
  EXPECT_EQ(span.cbegin(), span.cend());
  EXPECT_EQ(span.rbegin(), span.rend());
  EXPECT_EQ(span.crbegin(), span.crend());
}

TEST(SpanTest, ConstructorDataSize) {
  constexpr const size_t kBufferSize = 42;
  std::vector<uint32_t> buffer(kBufferSize);
  uint32_t* data = buffer.data();

  berrydb::span<uint32_t> span(data, kBufferSize);
  static_assert(
      is_same_v<uint32_t*, decltype(span.data())>,
      "data() type correctness");
  EXPECT_EQ(data, span.data());
  EXPECT_EQ(kBufferSize, span.size());
  EXPECT_EQ(kBufferSize * 4, span.size_bytes());
  EXPECT_FALSE(span.empty());

  berrydb::span<uint32_t> empty_span(data, 0);
  EXPECT_EQ(0U, empty_span.size());
  EXPECT_EQ(0U, empty_span.size_bytes());
  EXPECT_TRUE(empty_span.empty());
}

TEST(SpanTest, MakeSpanDataSize) {
  constexpr const size_t kBufferSize = 42;
  std::vector<uint32_t> buffer(kBufferSize);
  uint32_t* data = buffer.data();

  berrydb::span<uint32_t> span = make_span(data, kBufferSize);
  static_assert(
      is_same_v<uint32_t*, decltype(span.data())>,
      "data() type correctness");
  EXPECT_EQ(data, span.data());
  EXPECT_EQ(kBufferSize, span.size());
  EXPECT_EQ(kBufferSize * 4, span.size_bytes());
  EXPECT_FALSE(span.empty());

  berrydb::span<uint32_t> empty_span = make_span(data, 0);
  EXPECT_EQ(0U, empty_span.size());
  EXPECT_EQ(0U, empty_span.size_bytes());
  EXPECT_TRUE(empty_span.empty());
}

TEST(SpanTest, ConstructorBeginEnd) {
  constexpr const size_t kBufferSize = 42;
  std::vector<uint32_t> buffer(kBufferSize);
  uint32_t* data = buffer.data();

  berrydb::span<uint32_t> span(data, data + kBufferSize);
  EXPECT_EQ(data, span.data());
  EXPECT_EQ(kBufferSize, span.size());
  EXPECT_EQ(kBufferSize * 4, span.size_bytes());
  EXPECT_FALSE(span.empty());

  berrydb::span<uint32_t> empty_span(data, data);
  EXPECT_EQ(0U, empty_span.size());
  EXPECT_EQ(0U, empty_span.size_bytes());
  EXPECT_TRUE(empty_span.empty());
}

TEST(SpanTest, MakeSpanBeginEnd) {
  constexpr const size_t kBufferSize = 42;
  std::vector<uint32_t> buffer(kBufferSize);
  uint32_t* data = buffer.data();

  berrydb::span<uint32_t> span = make_span(data, data + kBufferSize);
  EXPECT_EQ(data, span.data());
  EXPECT_EQ(kBufferSize, span.size());
  EXPECT_EQ(kBufferSize * 4, span.size_bytes());
  EXPECT_FALSE(span.empty());

  berrydb::span<uint32_t> empty_span = make_span(data, data);
  EXPECT_EQ(0U, empty_span.size());
  EXPECT_EQ(0U, empty_span.size_bytes());
  EXPECT_TRUE(empty_span.empty());
}

TEST(SpanTest, ConstructorStaticArray) {
  uint32_t buffer[6] = { 1, 2, 3, 4, 5, 6 };
  berrydb::span<uint32_t> span(buffer);

  EXPECT_EQ(buffer, span.data());
  EXPECT_EQ(6U, span.size());
  EXPECT_EQ(6U * 4, span.size_bytes());
  EXPECT_FALSE(span.empty());
}

TEST(SpanTest, MakeSpanStaticArray) {
  uint32_t buffer[6] = { 1, 2, 3, 4, 5, 6 };
  berrydb::span<uint32_t> span = make_span(buffer);

  EXPECT_EQ(buffer, span.data());
  EXPECT_EQ(6U, span.size());
  EXPECT_EQ(6U * 4, span.size_bytes());
  EXPECT_FALSE(span.empty());
}

TEST(SpanTest, ConstructorCopy) {
  constexpr const size_t kBufferSize = 42;
  std::vector<uint32_t> buffer(kBufferSize);
  uint32_t* data = buffer.data();
  berrydb::span<uint32_t> span(data, kBufferSize);
  berrydb::span<uint32_t> span_copy = span;

  EXPECT_EQ(data, span_copy.data());
  EXPECT_EQ(kBufferSize, span_copy.size());
  EXPECT_EQ(kBufferSize * 4, span_copy.size_bytes());
  EXPECT_FALSE(span_copy.empty());
}

TEST(SpanTest, ConstructorCopyFromNonConst) {
  constexpr const size_t kBufferSize = 42;
  std::vector<uint32_t> buffer(kBufferSize);
  uint32_t* data = buffer.data();
  berrydb::span<uint32_t> span(data, kBufferSize);
  berrydb::span<const uint32_t> span_copy = span;

  static_assert(
      is_same_v<const uint32_t*, decltype(span_copy.data())>,
      "data() type correctness");

  EXPECT_EQ(data, span_copy.data());
  EXPECT_EQ(kBufferSize, span_copy.size());
  EXPECT_EQ(kBufferSize * 4, span_copy.size_bytes());
  EXPECT_FALSE(span_copy.empty());
}

TEST(SpanTest, First) {
  uint32_t buffer[6] = { 1, 2, 3, 4, 5, 6 };
  berrydb::span<uint32_t> span(buffer);

  EXPECT_EQ(0U, span.first(0).size());
  EXPECT_EQ(0U, span.first(0).size_bytes());
  EXPECT_TRUE(span.first(0).empty());

  EXPECT_EQ(buffer, span.first(1).data());
  EXPECT_EQ(1U, span.first(1).size());
  EXPECT_EQ(1U * 4, span.first(1).size_bytes());
  EXPECT_FALSE(span.first(1).empty());

  EXPECT_EQ(buffer, span.first(3).data());
  EXPECT_EQ(3U, span.first(3).size());
  EXPECT_EQ(3U * 4, span.first(3).size_bytes());
  EXPECT_FALSE(span.first(3).empty());

  EXPECT_EQ(buffer, span.first(6).data());
  EXPECT_EQ(6U, span.first(6).size());
  EXPECT_EQ(6U * 4, span.first(6).size_bytes());
  EXPECT_FALSE(span.first(6).empty());
}

TEST(SpanTest, Last) {
  uint32_t buffer[6] = { 1, 2, 3, 4, 5, 6 };
  berrydb::span<uint32_t> span(buffer);

  EXPECT_EQ(0U, span.last(0).size());
  EXPECT_EQ(0U, span.last(0).size_bytes());
  EXPECT_TRUE(span.last(0).empty());

  EXPECT_EQ(buffer + 5, span.last(1).data());
  EXPECT_EQ(1U, span.last(1).size());
  EXPECT_EQ(1U * 4, span.last(1).size_bytes());
  EXPECT_FALSE(span.last(1).empty());

  EXPECT_EQ(buffer + 3, span.last(3).data());
  EXPECT_EQ(3U, span.last(3).size());
  EXPECT_EQ(3U * 4, span.last(3).size_bytes());
  EXPECT_FALSE(span.last(3).empty());

  EXPECT_EQ(buffer, span.last(6).data());
  EXPECT_EQ(6U, span.last(6).size());
  EXPECT_EQ(6U * 4, span.last(6).size_bytes());
  EXPECT_FALSE(span.last(6).empty());
}

TEST(SpanTest, Subspan) {
  uint32_t buffer[6] = { 1, 2, 3, 4, 5, 6 };
  berrydb::span<uint32_t> span(buffer);

  EXPECT_EQ(0U, span.subspan(0, 0).size());
  EXPECT_EQ(0U, span.subspan(0, 0).size_bytes());
  EXPECT_TRUE(span.subspan(0, 0).empty());

  EXPECT_EQ(buffer, span.subspan(0, 2).data());
  EXPECT_EQ(2U, span.subspan(0, 2).size());
  EXPECT_EQ(2U * 4, span.subspan(0, 2).size_bytes());
  EXPECT_FALSE(span.subspan(0, 2).empty());

  EXPECT_EQ(buffer, span.subspan(0, 6).data());
  EXPECT_EQ(6U, span.subspan(0, 6).size());
  EXPECT_EQ(6U * 4, span.subspan(0, 6).size_bytes());
  EXPECT_FALSE(span.subspan(0, 6).empty());

  EXPECT_EQ(0U, span.subspan(2, 0).size());
  EXPECT_EQ(0U, span.subspan(2, 0).size_bytes());
  EXPECT_TRUE(span.subspan(2, 0).empty());

  EXPECT_EQ(buffer + 2, span.subspan(2, 3).data());
  EXPECT_EQ(3U, span.subspan(2, 3).size());
  EXPECT_EQ(3U * 4, span.subspan(2, 3).size_bytes());
  EXPECT_FALSE(span.subspan(2, 3).empty());

  EXPECT_EQ(buffer + 2, span.subspan(2, 4).data());
  EXPECT_EQ(4U, span.subspan(2, 4).size());
  EXPECT_EQ(4U * 4, span.subspan(2, 4).size_bytes());
  EXPECT_FALSE(span.subspan(2, 4).empty());

  EXPECT_EQ(0U, span.subspan(6, 0).size());
  EXPECT_EQ(0U, span.subspan(6, 0).size_bytes());
  EXPECT_TRUE(span.subspan(6, 0).empty());

  EXPECT_EQ(buffer, span.subspan(0, static_cast<size_t>(-1)).data());
  EXPECT_EQ(6U, span.subspan(0, static_cast<size_t>(-1)).size());
  EXPECT_EQ(6U * 4, span.subspan(0, static_cast<size_t>(-1)).size_bytes());
  EXPECT_FALSE(span.subspan(0, static_cast<size_t>(-1)).empty());

  EXPECT_EQ(buffer + 2, span.subspan(2, static_cast<size_t>(-1)).data());
  EXPECT_EQ(4U, span.subspan(2, static_cast<size_t>(-1)).size());
  EXPECT_EQ(4U * 4, span.subspan(2, static_cast<size_t>(-1)).size_bytes());
  EXPECT_FALSE(span.subspan(2, static_cast<size_t>(-1)).empty());

  EXPECT_EQ(0U, span.subspan(6, static_cast<size_t>(-1)).size());
  EXPECT_EQ(0U, span.subspan(6, static_cast<size_t>(-1)).size_bytes());
  EXPECT_TRUE(span.subspan(6, static_cast<size_t>(-1)).empty());
}

TEST(SpanTest, ElementAccessor) {
  constexpr const size_t kBufferSize = 42;
  std::vector<uint32_t> buffer(kBufferSize);
  uint32_t* data = buffer.data();
  berrydb::span<uint32_t> span(data, kBufferSize);

  for (size_t i = 0; i < kBufferSize; ++i) {
    buffer[i] = static_cast<uint32_t>(3 * i * i + 42 * i + 7);

    EXPECT_EQ(buffer[i], span[i]);
    EXPECT_EQ(buffer[i], span(i));
  }
}

TEST(SpanTest, SameSpanComparisons) {
  uint32_t buffer[6] = { 1, 2, 3, 4, 5, 6 };

  berrydb::span<uint32_t> span(buffer);
  ASSERT_TRUE(span == span);
  ASSERT_FALSE(span != span);
  ASSERT_TRUE(span <= span);
  ASSERT_FALSE(span < span);
  ASSERT_TRUE(span >= span);
  ASSERT_FALSE(span > span);

  berrydb::span<uint32_t> span_copy = span;
  ASSERT_TRUE(span == span_copy);
  ASSERT_FALSE(span != span_copy);
  ASSERT_TRUE(span <= span_copy);
  ASSERT_FALSE(span < span_copy);
  ASSERT_TRUE(span >= span_copy);
  ASSERT_FALSE(span > span_copy);
}

TEST(SpanTest, DifferentSpanComparisons) {
  uint32_t buffer[6] = { 1, 2, 3, 4, 5, 6 };
  berrydb::span<uint32_t> span(buffer);

  berrydb::span<uint32_t> prefix_span = span.first(4);
  ASSERT_FALSE(span == prefix_span);
  ASSERT_TRUE(span != prefix_span);
  ASSERT_FALSE(span <= prefix_span);
  ASSERT_FALSE(span < prefix_span);
  ASSERT_TRUE(span >= prefix_span);
  ASSERT_TRUE(span > prefix_span);

  uint32_t other_buffer[5] = { 2, 3, 4, 5, 6 };
  berrydb::span<uint32_t> other_span(other_buffer);
  ASSERT_FALSE(span == other_span);
  ASSERT_TRUE(span != other_span);
  ASSERT_TRUE(span <= other_span);
  ASSERT_TRUE(span < other_span);
  ASSERT_FALSE(span >= other_span);
  ASSERT_FALSE(span > other_span);
}

TEST(SpanTest, EmptySpanComparisons) {
  berrydb::span<uint32_t> empty_span;
  ASSERT_TRUE(empty_span == empty_span);
  ASSERT_FALSE(empty_span != empty_span);
  ASSERT_TRUE(empty_span <= empty_span);
  ASSERT_FALSE(empty_span < empty_span);
  ASSERT_TRUE(empty_span >= empty_span);
  ASSERT_FALSE(empty_span > empty_span);

  uint32_t buffer[6] = { 1, 2, 3, 4, 5, 6 };
  berrydb::span<uint32_t> span(buffer);
  ASSERT_FALSE(span == empty_span);
  ASSERT_TRUE(span != empty_span);
  ASSERT_FALSE(span <= empty_span);
  ASSERT_FALSE(span < empty_span);
  ASSERT_TRUE(span >= empty_span);
  ASSERT_TRUE(span > empty_span);
}

}  // namespace berrydb
