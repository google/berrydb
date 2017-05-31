// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "./store_impl.h"

#include <cstring>
#include <random>

#include "gtest/gtest.h"

#include "berrydb/options.h"
#include "./page_pool.h"
#include "./pool_impl.h"
#include "./test/block_access_file_wrapper.h"

namespace berrydb {

class StoreImplTest : public ::testing::Test {
 protected:

  virtual void SetUp() {
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

  virtual void TearDown() {
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
  std::mt19937 rnd_;
};

TEST_F(StoreImplTest, Constructor) {
  CreatePool(kStorePageShift, 1);

  PagePool* page_pool = pool_->page_pool();
  StoreImpl* store = StoreImpl::Create(
      data_file_, data_file_size_, log_file_, log_file_size_, page_pool,
      StoreOptions());

  EXPECT_FALSE(store->IsClosed());
  EXPECT_EQ(0U, page_pool->allocated_pages());

  EXPECT_EQ(Status::kSuccess, store->Close());
  EXPECT_TRUE(store->IsClosed());
  EXPECT_EQ(0U, page_pool->allocated_pages());
}

TEST_F(StoreImplTest, WriteReadPage) {
  uint8_t buffer[4 << kStorePageShift];
  for (size_t i = 0; i < sizeof(buffer); ++i)
    buffer[i] = static_cast<uint8_t>(rnd_());

  CreatePool(kStorePageShift, 2);
  PagePool* page_pool = pool_->page_pool();
  StoreImpl* store = StoreImpl::Create(
      data_file_, data_file_size_, log_file_, log_file_size_, page_pool,
      StoreOptions());

  Page* page = page_pool->AllocPage();
  ASSERT_TRUE(page != nullptr);

  for (size_t i = 0; i < 4; ++i) {
    ASSERT_EQ(Status::kSuccess, page_pool->AssignPageToStore(
        page, store, i, PagePool::PageFetchMode::kIgnorePageData));
    page->MarkDirty();
    std::memcpy(
        page->data(), buffer + (i << kStorePageShift), 1 << kStorePageShift);
    ASSERT_EQ(Status::kSuccess, store->WritePage(page));
    EXPECT_TRUE(page->is_dirty());

    // Clear the page to make sure ReadPage fetches the correct content.
    std::memset(page->data(), 0, 1 << kStorePageShift);
    page->MarkDirty(false);  // Bypass DCHECKs in ReadPage.
    ASSERT_EQ(Status::kSuccess, store->ReadPage(page));
    ASSERT_EQ(0, std::memcmp(
        page->data(), buffer + (i << kStorePageShift), 1 << kStorePageShift));

    page_pool->UnassignPageFromStore(page);
    ASSERT_TRUE(!page->IsUnpinned());
  }

  for (size_t i = 0; i < 4; ++i) {
    ASSERT_EQ(Status::kSuccess, page_pool->AssignPageToStore(
        page, store, i, PagePool::PageFetchMode::kIgnorePageData));
    page->MarkDirty();

    // Clear the page to make sure ReadPage fetches the correct content.
    std::memset(page->data(), 0, 1 << kStorePageShift);
    page->MarkDirty(false);  // Bypass DCHECKs in ReadPage.
    ASSERT_EQ(Status::kSuccess, store->ReadPage(page));
    ASSERT_EQ(0, std::memcmp(
        page->data(), buffer + (i << kStorePageShift), 1 << kStorePageShift));

    page_pool->UnassignPageFromStore(page);
    ASSERT_TRUE(!page->IsUnpinned());
  }

  EXPECT_EQ(Status::kSuccess, store->Close());
  EXPECT_TRUE(store->IsClosed());

  page_pool->UnpinUnassignedPage(page);
  EXPECT_TRUE(page->IsUnpinned());
  store->Release();
}

TEST_F(StoreImplTest, CloseUnassignsPages) {
  CreatePool(kStorePageShift, 16);
  PagePool* page_pool = pool_->page_pool();
  StoreImpl* store = StoreImpl::Create(
      data_file_, data_file_size_, log_file_, log_file_size_, page_pool,
      StoreOptions());

  Page* page[4];
  for (size_t i = 0; i < 4; ++i) {
    page[i] = page_pool->AllocPage();
    ASSERT_TRUE(page[i] != nullptr);
  }

  for (size_t i = 0; i < 4; ++i) {
    ASSERT_EQ(Status::kSuccess, page_pool->AssignPageToStore(
        page[i], store, i, PagePool::PageFetchMode::kIgnorePageData));
    page[i]->MarkDirty(false);  // Avoid writing the page to disk.

    page_pool->UnpinStorePage(page[i]);
    EXPECT_TRUE(page[i]->IsUnpinned());
  }

  EXPECT_EQ(4U, page_pool->allocated_pages());
  EXPECT_EQ(0U, page_pool->unused_pages());
  EXPECT_EQ(0U, page_pool->pinned_pages());

  ASSERT_EQ(Status::kSuccess, store->Close());
  EXPECT_EQ(4U, page_pool->allocated_pages());
  EXPECT_EQ(4U, page_pool->unused_pages());
  EXPECT_EQ(0U, page_pool->pinned_pages());

  store->Release();
}

}  // namespace berrydb
