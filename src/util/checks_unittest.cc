// Copyright 2018 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "./checks.h"

#include "gtest/gtest.h"

namespace berrydb {

class ChecksTest : public ::testing::Test {
 public:
  void SetUp() override {
    // The initialization is not done in a constructor to avoid having compilers
    // optimize out failing / passing CHECKs at compile time.
    five_ = 5;
    six_ = 6;
    seven_ = 7;
  }
 protected:
  int five_, six_, seven_;
};

using ChecksDeathTest = ChecksTest;

TEST_F(ChecksTest, CheckSuccess) {
  ASSERT_TRUE(five_ == six_ - 1);
  BERRYDB_CHECK(five_ == six_ - 1);
}

TEST_F(ChecksDeathTest, CheckFailure) {
  ASSERT_FALSE(five_ == six_);
#if BERRYDB_CHECK_IS_ON()
  EXPECT_DEATH_IF_SUPPORTED(BERRYDB_CHECK(five_ == six_), "");
#endif  // BERRYDB_CHECK_IS_ON()
}

TEST_F(ChecksTest, CheckEqSuccess) {
  ASSERT_EQ(five_, six_ - 1);
  BERRYDB_CHECK_EQ(five_, six_ - 1);
}

TEST_F(ChecksDeathTest, CheckEqFailure) {
  ASSERT_NE(five_, six_);
#if BERRYDB_CHECK_IS_ON()
  EXPECT_DEATH_IF_SUPPORTED(BERRYDB_CHECK_EQ(five_, six_), "");
  EXPECT_DEATH_IF_SUPPORTED(BERRYDB_CHECK_EQ(six_, five_), "");
#endif  // BERRYDB_CHECK_IS_ON()
}

TEST_F(ChecksTest, CheckNeSuccess) {
  ASSERT_NE(five_, six_);
  ASSERT_NE(six_, five_);
  BERRYDB_CHECK_NE(five_, six_);
  BERRYDB_CHECK_NE(six_, five_);
}

TEST_F(ChecksDeathTest, CheckNeFailure) {
  ASSERT_EQ(five_, six_ - 1);
#if BERRYDB_CHECK_IS_ON()
  EXPECT_DEATH_IF_SUPPORTED(BERRYDB_CHECK_NE(five_, six_ - 1), "");
#endif  // BERRYDB_CHECK_IS_ON()
}

TEST_F(ChecksTest, CheckGeSuccess) {
  ASSERT_GE(six_, five_);
  ASSERT_GE(six_, five_ + 1);
  BERRYDB_CHECK_GE(six_, five_);
  BERRYDB_CHECK_GE(six_, five_ + 1);
}

TEST_F(ChecksDeathTest, CheckGeFailure) {
  ASSERT_LT(five_, six_);
#if BERRYDB_CHECK_IS_ON()
  EXPECT_DEATH_IF_SUPPORTED(BERRYDB_CHECK_GE(five_, six_), "");
#endif  // BERRYDB_CHECK_IS_ON()
}

TEST_F(ChecksTest, CheckGtSuccess) {
  ASSERT_GT(six_, five_);
  BERRYDB_CHECK_GT(six_, five_);
}

TEST_F(ChecksDeathTest, CheckGtFailure) {
  ASSERT_LE(five_, six_);
  ASSERT_LE(five_ + 1, six_);
#if BERRYDB_CHECK_IS_ON()
  EXPECT_DEATH_IF_SUPPORTED(BERRYDB_CHECK_GT(five_, six_), "");
  EXPECT_DEATH_IF_SUPPORTED(BERRYDB_CHECK_GT(five_ + 1, six_), "");
#endif  // BERRYDB_CHECK_IS_ON()
}

TEST_F(ChecksTest, CheckLeSuccess) {
  ASSERT_LE(five_, six_);
  ASSERT_LE(five_ + 1, six_);
  BERRYDB_CHECK_LE(five_, six_);
  BERRYDB_CHECK_LE(five_ + 1, six_);
}

TEST_F(ChecksDeathTest, CheckLeFailure) {
  ASSERT_GT(six_, five_);
#if BERRYDB_CHECK_IS_ON()
  EXPECT_DEATH_IF_SUPPORTED(BERRYDB_CHECK_LE(six_, five_), "");
#endif  // BERRYDB_CHECK_IS_ON()
}

TEST_F(ChecksTest, CheckLtSuccess) {
  ASSERT_LT(five_, six_);
  BERRYDB_CHECK_LT(five_, six_);
}

TEST_F(ChecksDeathTest, CheckLtFailure) {
  ASSERT_GE(six_, five_);
  ASSERT_GE(six_, five_ + 1);
#if BERRYDB_CHECK_IS_ON()
  EXPECT_DEATH_IF_SUPPORTED(BERRYDB_CHECK_LT(six_, five_), "");
  EXPECT_DEATH_IF_SUPPORTED(BERRYDB_CHECK_LT(six_, five_ + 1), "");
#endif  // BERRYDB_CHECK_IS_ON()
}

TEST_F(ChecksTest, AssumeSuccess) {
  ASSERT_TRUE(five_ == six_ - 1);
  BERRYDB_ASSUME(five_ == six_ - 1);
}

TEST_F(ChecksDeathTest, AssumeFailure) {
  ASSERT_FALSE(five_ == six_);
#if BERRYDB_CHECK_IS_ON()
  EXPECT_DEATH_IF_SUPPORTED(BERRYDB_ASSUME(five_ == six_), "");
#endif  // BERRYDB_CHECK_IS_ON()
}

TEST_F(ChecksTest, AssumeEqSuccess) {
  ASSERT_EQ(five_, six_ - 1);
  BERRYDB_ASSUME_EQ(five_, six_ - 1);
}

TEST_F(ChecksDeathTest, AssumeEqFailure) {
  ASSERT_NE(five_, six_);
#if BERRYDB_CHECK_IS_ON()
  EXPECT_DEATH_IF_SUPPORTED(BERRYDB_ASSUME_EQ(five_, six_), "");
  EXPECT_DEATH_IF_SUPPORTED(BERRYDB_ASSUME_EQ(six_, five_), "");
#endif  // BERRYDB_CHECK_IS_ON()
}

TEST_F(ChecksTest, AssumeNeSuccess) {
  ASSERT_NE(five_, six_);
  ASSERT_NE(six_, five_);
  BERRYDB_ASSUME_NE(five_, six_);
  BERRYDB_ASSUME_NE(six_, five_);
}

TEST_F(ChecksDeathTest, AssumeNeFailure) {
  ASSERT_EQ(five_, six_ - 1);
#if BERRYDB_CHECK_IS_ON()
  EXPECT_DEATH_IF_SUPPORTED(BERRYDB_ASSUME_NE(five_, six_ - 1), "");
#endif  // BERRYDB_CHECK_IS_ON()
}

TEST_F(ChecksTest, AssumeGeSuccess) {
  ASSERT_GE(six_, five_);
  ASSERT_GE(six_, five_ + 1);
  BERRYDB_ASSUME_GE(six_, five_);
  BERRYDB_ASSUME_GE(six_, five_ + 1);
}

TEST_F(ChecksDeathTest, AssumeGeFailure) {
  ASSERT_LT(five_, six_);
#if BERRYDB_CHECK_IS_ON()
  EXPECT_DEATH_IF_SUPPORTED(BERRYDB_ASSUME_GE(five_, six_), "");
#endif  // BERRYDB_CHECK_IS_ON()
}

TEST_F(ChecksTest, AssumeGtSuccess) {
  ASSERT_GT(six_, five_);
  BERRYDB_ASSUME_GT(six_, five_);
}

TEST_F(ChecksDeathTest, AssumeGtFailure) {
  ASSERT_LE(five_, six_);
  ASSERT_LE(five_ + 1, six_);
#if BERRYDB_CHECK_IS_ON()
  EXPECT_DEATH_IF_SUPPORTED(BERRYDB_ASSUME_GT(five_, six_), "");
  EXPECT_DEATH_IF_SUPPORTED(BERRYDB_ASSUME_GT(five_ + 1, six_), "");
#endif  // BERRYDB_CHECK_IS_ON()
}

TEST_F(ChecksTest, AssumeLeSuccess) {
  ASSERT_LE(five_, six_);
  ASSERT_LE(five_ + 1, six_);
  BERRYDB_ASSUME_LE(five_, six_);
  BERRYDB_ASSUME_LE(five_ + 1, six_);
}

TEST_F(ChecksDeathTest, AssumeLeFailure) {
  ASSERT_GT(six_, five_);
#if BERRYDB_CHECK_IS_ON()
  EXPECT_DEATH_IF_SUPPORTED(BERRYDB_ASSUME_LE(six_, five_), "");
#endif  // BERRYDB_CHECK_IS_ON()
}

TEST_F(ChecksTest, AssumeLtSuccess) {
  ASSERT_LT(five_, six_);
  BERRYDB_ASSUME_LT(five_, six_);
}

TEST_F(ChecksDeathTest, AssumeLtFailure) {
  ASSERT_GE(six_, five_);
  ASSERT_GE(six_, five_ + 1);
#if BERRYDB_CHECK_IS_ON()
  EXPECT_DEATH_IF_SUPPORTED(BERRYDB_ASSUME_LT(six_, five_), "");
  EXPECT_DEATH_IF_SUPPORTED(BERRYDB_ASSUME_LT(six_, five_ + 1), "");
#endif  // BERRYDB_CHECK_IS_ON()
}

TEST_F(ChecksDeathTest, Unreachable) {
#if BERRYDB_CHECK_IS_ON()
  EXPECT_DEATH_IF_SUPPORTED(BERRYDB_UNREACHABLE(), "");
  EXPECT_DEATH_IF_SUPPORTED(BERRYDB_UNREACHABLE(), "");
#endif  // BERRYDB_CHECK_IS_ON()
}

}  // namespace berrydb
