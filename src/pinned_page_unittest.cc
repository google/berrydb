// Copyright 2018 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "./pinned_page.h"

#include "berrydb/options.h"
#include "berrydb/span.h"
#include "berrydb/types.h"
#include "./page_pool.h"
#include "./pool_impl.h"
#include "./util/unique_ptr.h"

#include "gtest/gtest.h"

namespace berrydb {

class PinnedPageTest : public ::testing::Test {
 protected:
  PinnedPageTest(){
    PoolOptions options;
    options.page_shift = kStorePageShift;
    options.page_pool_size = 1;
    pool_.reset(PoolImpl::Create(options));

    page_ = Page::Create(pool_->page_pool());
  }

  ~PinnedPageTest() {
    page_->Release(pool_->page_pool());
  }

  constexpr static size_t kStorePageShift = 12;

  UniquePtr<PoolImpl> pool_;
  Page* page_;
};

TEST_F(PinnedPageTest, UnpinOnDestruction) {
  ASSERT_TRUE(!page_->IsUnpinned());
  {
    PinnedPage pinned_page(page_, pool_->page_pool());
    EXPECT_TRUE(!page_->IsUnpinned());
  }
  EXPECT_FALSE(page_->IsUnpinned());
}

TEST_F(PinnedPageTest, Get) {
  PinnedPage pinned_page(page_, pool_->page_pool());

  ASSERT_EQ(page_, pinned_page.get());
}

TEST_F(PinnedPageTest, SmartPointerOperators) {
  PinnedPage pinned_page(page_, pool_->page_pool());

  ASSERT_EQ(page_, &(*pinned_page));
  ASSERT_EQ(page_->buffer(), pinned_page->buffer());
}

TEST_F(PinnedPageTest, Data) {
  PinnedPage pinned_page(page_, pool_->page_pool());

  span<const uint8_t> golden = page_->data(1 << kStorePageShift);
  span<const uint8_t> data = pinned_page.data();
  EXPECT_EQ(golden.size(), data.size());
  EXPECT_EQ(golden.data(), data.data());
}

TEST_F(PinnedPageTest, MutableData) {
  PinnedPage pinned_page(page_, pool_->page_pool());

  span<uint8_t> golden = page_->mutable_data(1 << kStorePageShift);
  span<uint8_t> data = pinned_page.mutable_data();
  EXPECT_EQ(golden.size(), data.size());
  EXPECT_EQ(golden.data(), data.data());
}

}  // namespace berrydb