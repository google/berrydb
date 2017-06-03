// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "./page.h"

#include "gtest/gtest.h"

#include "berrydb/options.h"
#include "berrydb/vfs.h"
#include "./page_pool.h"
#include "./pool_impl.h"
#include "./store_impl.h"

namespace berrydb {

class PageTest : public ::testing::Test {
 protected:
  void SetUp() override {
    vfs_ = DefaultVfs();
    vfs_->DeleteFile(kStoreFileName);
    vfs_->DeleteFile(StoreImpl::LogFilePath(kStoreFileName));
    pool_ = nullptr;

    ASSERT_EQ(
        Status::kSuccess,
        vfs_->OpenForBlockAccess(kStoreFileName, kStorePageShift, true, false,
        &data_file_, &data_file_size_));
    ASSERT_EQ(
        Status::kSuccess,
        vfs_->OpenForRandomAccess(StoreImpl::LogFilePath(kStoreFileName),
        true, false, &log_file_, &log_file_size_));
  }

  void TearDown() override {
    if (pool_ != nullptr)
      pool_->Release();

    vfs_->DeleteFile(kStoreFileName);
    vfs_->DeleteFile(StoreImpl::LogFilePath(kStoreFileName));

  }

  const std::string kStoreFileName = "test_store_impl.berry";
  constexpr static size_t kStorePageShift = 12;

  void CreatePool(int page_shift, int page_capacity) {
    PoolOptions options;
    options.page_shift = page_shift;
    options.page_pool_size = page_capacity;
    pool_ = PoolImpl::Create(options);
  }

  Vfs* vfs_;
  PoolImpl* pool_;
  BlockAccessFile* data_file_;
  size_t data_file_size_;
  RandomAccessFile* log_file_;
  size_t log_file_size_;
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

TEST_F(PageTest, AssignToStoreUnassignFromStore) {
  CreatePool(kStorePageShift, 42);
  PagePool* page_pool = pool_->page_pool();
  StoreImpl* store = StoreImpl::Create(
      data_file_, data_file_size_, log_file_, log_file_size_, page_pool,
      StoreOptions());

  Page* page = Page::Create(page_pool);
  ASSERT_TRUE(!page->IsUnpinned());

  page->AssignToStore(store, 1337);
  store->PageAssigned(page);
  EXPECT_EQ(store, page->store());
  EXPECT_EQ(1337U, page->page_id());

  page->UnassignFromStore();
  store->PageUnassigned(page);
#if DCHECK_IS_ON()
  EXPECT_EQ(nullptr, page->store());
#endif  // DCHECK_IS_ON()

  page->Release(page_pool);
}

}  // namespace berrydb
