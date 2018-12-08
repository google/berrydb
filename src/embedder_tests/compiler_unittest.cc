// Copyright 2018 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "berrydb/platform.h"

#include "gtest/gtest.h"

namespace berrydb {

namespace {

NODISCARD int NoDiscardCompilationTest() {
  return 42;
}

// Checks that code using MAYBE_UNUSED compiles correctly.
class MaybeUnusedCompilationTest {
 public:
  // Targets GCC bug that only triggers on the first argument in a constructor.
  // See https://gcc.gnu.org/bugzilla/show_bug.cgi?id=81429
  explicit MaybeUnusedCompilationTest(MAYBE_UNUSED int unused_argument) {}

  void UnusedLocal() {
    // The compiler should not complain about the unused local.
    MAYBE_UNUSED int unused_local = 5;
  }

  // The compiler should not complain about the unused argument.
  void UnusedArgument(MAYBE_UNUSED int unused_argument) {}
};

int UnreachableCompilationTest(int argument) {
  switch (argument) {
    case 1:
      return 2;
    case 3:
      return 5;
  }

  // The compiler should not complain about the missing return value, as this
  // code path is flagged unreachable.
  UNREACHABLE();
}

int AssumeCompilationTest(int argument) {
  ASSUME(argument == 5);
  return argument + 3;
}

int LikelyEqualsFive(int value) {
  // LIKELY is not the same as ASSUME. The compiler should generate code
  // handling both cases.
  if (LIKELY(value == 5)) {
    return 1;
  }
  return 0;
}

int UnlikelyEqualsFive(int value) {
  // UNLIKELY is not the same as ASSUME. The compiler should generate code
  // handling both cases.
  if (UNLIKELY(value == 5)) {
    return 1;
  }
  return 0;
}

ALWAYS_INLINE int AlwaysInline() {
  return 42;
}

NOINLINE int NoInline() {
  return 5;
}

}  // namespace

TEST(CompilerTest, NoDiscardCompiles) {
  EXPECT_EQ(42, NoDiscardCompilationTest());
}

TEST(CompilerTest, MaybeUnusedCompiles) {
  MaybeUnusedCompilationTest test_instance(42);
  test_instance.UnusedLocal();
  test_instance.UnusedArgument(42);
}

TEST(CompilerTest, UnreachableCompiles) {
  EXPECT_EQ(2, UnreachableCompilationTest(1));
  EXPECT_EQ(5, UnreachableCompilationTest(3));
}

TEST(CompilerTest, AssumeCompiles) {
  EXPECT_EQ(8, AssumeCompilationTest(5));
}

TEST(CompilerTest, LikelyIsNotAssume) {
  EXPECT_EQ(1, LikelyEqualsFive(5));
  EXPECT_EQ(0, LikelyEqualsFive(4));
  EXPECT_EQ(0, LikelyEqualsFive(6));
}

TEST(CompilerTest, UnlikelyIsNotAssume) {
  EXPECT_EQ(1, UnlikelyEqualsFive(5));
  EXPECT_EQ(0, UnlikelyEqualsFive(4));
  EXPECT_EQ(0, UnlikelyEqualsFive(6));
}

TEST(CompilerTest, AlwaysInlineCompiles) {
  EXPECT_EQ(42, AlwaysInline());
}

TEST(CompilerTest, NoInlineCompiles) {
  EXPECT_EQ(5, NoInline());
}

}  // namespace berrydb
