// Copyright 2018 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "berrydb/platform.h"

#include "gtest/gtest.h"

namespace berrydb {

class DcheckTest : public ::testing::Test {
 public:
  void SetUp() override {
    // The initialization is not done in a constructor to avoid having compilers
    // optimize out failing / passing DCHECKs at compile time.
    five_ = 5;
    six_ = 6;
    seven_ = 7;
  }
 protected:
  int five_, six_, seven_;
};

using DcheckDeathTest = DcheckTest;

TEST_F(DcheckTest, DcheckSuccess) {
  ASSERT_TRUE(five_ == six_ - 1);
  DCHECK(five_ == six_ - 1);
}

TEST_F(DcheckDeathTest, DcheckFailure) {
  ASSERT_FALSE(five_ == six_);
#if DCHECK_IS_ON()
  EXPECT_DEATH_IF_SUPPORTED(DCHECK(five_ == six_), "");
#endif  // DCHECK_IS_ON()
}

TEST_F(DcheckTest, DcheckEqSuccess) {
  ASSERT_EQ(five_, six_ - 1);
  DCHECK_EQ(five_, six_ - 1);
}

TEST_F(DcheckDeathTest, DcheckEqFailure) {
  ASSERT_NE(five_, six_);
#if DCHECK_IS_ON()
  EXPECT_DEATH_IF_SUPPORTED(DCHECK_EQ(five_, six_), "");
#endif  // DCHECK_IS_ON()
}

TEST_F(DcheckTest, DcheckNeSuccess) {
  ASSERT_NE(five_, six_);
  DCHECK_NE(five_, six_);
}

TEST_F(DcheckDeathTest, DcheckNeFailure) {
  ASSERT_EQ(five_, six_ - 1);
#if DCHECK_IS_ON()
  EXPECT_DEATH_IF_SUPPORTED(DCHECK_NE(five_, six_ - 1), "");
#endif  // DCHECK_IS_ON()
}

TEST_F(DcheckTest, DcheckGeSuccess) {
  ASSERT_GE(six_, five_);
  DCHECK_GE(six_, five_);
}

TEST_F(DcheckDeathTest, DcheckGeFailure) {
  ASSERT_LT(five_, six_);
#if DCHECK_IS_ON()
  EXPECT_DEATH_IF_SUPPORTED(DCHECK_GE(five_, six_), "");
#endif  // DCHECK_IS_ON()
}

TEST_F(DcheckTest, DcheckGtSuccess) {
  ASSERT_GT(six_, five_);
  DCHECK_GT(six_, five_);
}

TEST_F(DcheckDeathTest, DcheckGtFailure) {
  ASSERT_LE(five_, six_);
#if DCHECK_IS_ON()
  EXPECT_DEATH_IF_SUPPORTED(DCHECK_GT(five_, six_), "");
#endif  // DCHECK_IS_ON()
}

TEST_F(DcheckTest, DcheckLeSuccess) {
  ASSERT_LE(five_, six_);
  DCHECK_LE(five_, six_);
}

TEST_F(DcheckDeathTest, DcheckLeFailure) {
  ASSERT_GT(six_, five_);
#if DCHECK_IS_ON()
  EXPECT_DEATH_IF_SUPPORTED(DCHECK_LE(six_, five_), "");
#endif  // DCHECK_IS_ON()
}

TEST_F(DcheckTest, DcheckLtSuccess) {
  ASSERT_LT(five_, six_);
  DCHECK_LT(five_, six_);
}

TEST_F(DcheckDeathTest, DcheckLtFailure) {
  ASSERT_GE(six_, five_);
#if DCHECK_IS_ON()
  EXPECT_DEATH_IF_SUPPORTED(DCHECK_LT(six_, five_), "");
#endif  // DCHECK_IS_ON()
}

}  // namespace berrydb
