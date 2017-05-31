// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "./page.h"

#include "gtest/gtest.h"

#include "berrydb/options.h"
#include "./page_pool.h"
#include "./pool_impl.h"

namespace berrydb {

class PageTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    pool_ = nullptr;
  }

  virtual void TearDown() {
    if (pool_ != nullptr)
      pool_->Release();
  }

  void CreatePool(int page_shift, int page_capacity) {
    PoolOptions options;
    options.page_shift = page_shift;
    options.page_pool_size = page_capacity;
    pool_ = PoolImpl::Create(options);
  }

  PoolImpl* pool_;
};

TEST_F(PageTest, CreateRelease) {
  CreatePool(12, 42);
  PagePool page_pool(pool_, 12, 42);

  Page* page = Page::Create(&page_pool);
#if DCHECK_IS_ON()
  EXPECT_EQ(nullptr, page->store());
  EXPECT_EQ(&page_pool, page->page_pool());
#endif  // DCHECK_IS_ON()

  page->RemovePin();
  EXPECT_TRUE(page->IsUnpinned());

  page->Release(&page_pool);
}

TEST_F(PageTest, Pinning) {
  CreatePool(12, 42);
  PagePool page_pool(pool_, 12, 42);

  Page* page = Page::Create(&page_pool);
  EXPECT_FALSE(page->IsUnpinned());
  page->RemovePin();
  EXPECT_TRUE(page->IsUnpinned());

  page->AddPin();
  EXPECT_FALSE(page->IsUnpinned());

  page->RemovePin();
  EXPECT_TRUE(page->IsUnpinned());

  page->AddPin();
  page->AddPin();
  EXPECT_FALSE(page->IsUnpinned());

  page->RemovePin();
  EXPECT_FALSE(page->IsUnpinned());
  page->RemovePin();
  EXPECT_TRUE(page->IsUnpinned());

  page->Release(&page_pool);
}

}  // namespace berrydb
