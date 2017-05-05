// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "./page_pool.h"

#include <string>

#include "gtest/gtest.h"

#include "berrydb/options.h"
#include "berrydb/store.h"
#include "berrydb/vfs.h"
#include "./pool_impl.h"
#include "./test/block_access_file_wrapper.h"

namespace berrydb {

class PagePoolTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    vfs_ = DefaultVfs();
    vfs_->DeleteFile(kStoreFileName1);
    vfs_->DeleteFile(kStoreFileName2);
    vfs_->DeleteFile(kStoreFileName3);
    pool_ = nullptr;
  }

  virtual void TearDown() {
    if (pool_ != nullptr)
      pool_->Release();

    vfs_->DeleteFile(kStoreFileName1);
    vfs_->DeleteFile(kStoreFileName2);
    vfs_->DeleteFile(kStoreFileName3);
  }

  void CreatePool(int page_shift, int page_capacity) {
    PoolOptions options;
    options.page_shift = page_capacity;
    options.page_pool_size = page_shift;
    pool_ = PoolImpl::Create(options);
  }

  const std::string kStoreFileName1 = "test_page_pool_1.berry";
  const std::string kStoreFileName2 = "test_page_pool_2.berry";
  const std::string kStoreFileName3 = "test_page_pool_3.berry";

  Vfs* vfs_;
  PoolImpl* pool_ ;
};

TEST_F(PagePoolTest, Constructor) {
  PagePool page_pool(16, 42);
  EXPECT_EQ(16U, page_pool.page_shift());
  EXPECT_EQ(65536U, page_pool.page_size());
  EXPECT_EQ(42U, page_pool.page_capacity());

  EXPECT_EQ(0U, page_pool.allocated_pages());
  EXPECT_EQ(0U, page_pool.unused_pages());
  EXPECT_EQ(0U, page_pool.pinned_pages());
}

TEST_F(PagePoolTest, AllocRespectsCapacity) {
  PagePool page_pool(12, 1);

  Page* page = page_pool.AllocPage();
  ASSERT_NE(nullptr, page);
  EXPECT_EQ(1U, page_pool.allocated_pages());
  EXPECT_EQ(0U, page_pool.unused_pages());
  EXPECT_EQ(1U, page_pool.pinned_pages());

  ASSERT_EQ(nullptr, page_pool.AllocPage());
  EXPECT_EQ(1U, page_pool.allocated_pages());
  EXPECT_EQ(0U, page_pool.unused_pages());
  EXPECT_EQ(1U, page_pool.pinned_pages());

  page_pool.UnpinUnassignedPage(page);
  EXPECT_EQ(1U, page_pool.allocated_pages());
  EXPECT_EQ(1U, page_pool.unused_pages());
  EXPECT_EQ(0U, page_pool.pinned_pages());
}

TEST_F(PagePoolTest, AllocUsesFreeList) {
  CreatePool(12, 1);
  PagePool* page_pool = pool_->page_pool();

  Page* page = page_pool->AllocPage();
  ASSERT_NE(nullptr, page);

  page_pool->UnpinUnassignedPage(page);
  EXPECT_EQ(1U, page_pool->allocated_pages());
  EXPECT_EQ(1U, page_pool->unused_pages());
  EXPECT_EQ(0U, page_pool->pinned_pages());

  Page* page2 = page_pool->AllocPage();
  EXPECT_EQ(page, page2);
  EXPECT_EQ(1U, page_pool->allocated_pages());
  EXPECT_EQ(0U, page_pool->unused_pages());
  EXPECT_EQ(1U, page_pool->pinned_pages());

  page_pool->UnpinUnassignedPage(page2);
}

TEST_F(PagePoolTest, AllocUsesLruList) {
  CreatePool(12, 1);
  PagePool* page_pool = pool_->page_pool();
  BlockAccessFile* file;
  ASSERT_EQ(
      Status::kSuccess,
      vfs_->OpenForBlockAccess(kStoreFileName1, 12, true, false, &file));
  StoreImpl* store = StoreImpl::Create(file, page_pool, StoreOptions());

  Page* page = page_pool->AllocPage();
  ASSERT_NE(nullptr, page);

  ASSERT_EQ(
      Status::kSuccess,
      page_pool->AssignPageToStore(page, store, 0, PagePool::kIgnorePageData));
  EXPECT_EQ(store, page->store());

  // Unset the page's dirty bit to avoid having the page written to the store
  // when it is evicted from the MRU list.
  page->MarkDirty(false);
  page_pool->UnpinStorePage(page);
  EXPECT_EQ(1U, page_pool->allocated_pages());
  EXPECT_EQ(0U, page_pool->unused_pages());
  EXPECT_EQ(0U, page_pool->pinned_pages());

  Page* page2 = page_pool->AllocPage();
  EXPECT_EQ(page, page2);
  EXPECT_EQ(1U, page_pool->allocated_pages());
  EXPECT_EQ(0U, page_pool->unused_pages());
  EXPECT_EQ(1U, page_pool->pinned_pages());
#if DCHECK_IS_ON()
  EXPECT_EQ(nullptr, page2->store());
#endif  // DCHECK_IS_ON()

  page_pool->UnpinUnassignedPage(page2);
}

TEST_F(PagePoolTest, UnassignFromStoreIoError) {
  CreatePool(12, 1);
  PagePool* page_pool = pool_->page_pool();
  BlockAccessFile* file;
  ASSERT_EQ(
      Status::kSuccess,
      vfs_->OpenForBlockAccess(kStoreFileName1, 12, true, false, &file));
  BlockAccessFileWrapper file_wrapper(file);
  StoreImpl* store =
      StoreImpl::Create(&file_wrapper, page_pool, StoreOptions());

  Page* page = page_pool->AllocPage();
  ASSERT_NE(nullptr, page);
  ASSERT_EQ(
      Status::kSuccess,
      page_pool->AssignPageToStore(page, store, 0, PagePool::kIgnorePageData));
  EXPECT_EQ(store, page->store());

  file_wrapper.SetAccessError(Status::kIoError);
  page_pool->UnassignPageFromStore(page);
#if DCHECK_IS_ON()
  EXPECT_EQ(nullptr, page->store());
#endif  // DCHECK_IS_ON()
  EXPECT_EQ(true, store->IsClosed());

  page_pool->UnpinUnassignedPage(page);
}

TEST_F(PagePoolTest, AssignToStoreSuccess) {

}

TEST_F(PagePoolTest, AssignToStoreIoError) {

}

}  // namespace berrydb
