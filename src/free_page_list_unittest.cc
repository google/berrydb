// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "./free_page_list.h"

#include <cstring>
#include <random>
#include <string>

#include "gtest/gtest.h"

#include "berrydb/options.h"
#include "berrydb/store.h"
#include "berrydb/vfs.h"
#include "./page_pool.h"
#include "./pool_impl.h"
#include "./store_impl.h"
#include "./test/block_access_file_wrapper.h"
#include "./test/file_deleter.h"
#include "./util/unique_ptr.h"

namespace berrydb {

class FreePageListTest : public ::testing::Test {
 protected:
  FreePageListTest()
      : vfs_(DefaultVfs()), data_file_deleter_(kStoreFileName),
        log_file_deleter_(StoreImpl::LogFilePath(kStoreFileName)) { }

  void SetUp() override {
    BlockAccessFile* raw_data_file;
    ASSERT_EQ(Status::kSuccess, vfs_->OpenForBlockAccess(
        data_file_deleter_.path(), kStorePageShift, true, false,
        &raw_data_file, &data_file_size_));
    data_file_.reset(raw_data_file);
    RandomAccessFile* raw_log_file;
    ASSERT_EQ(Status::kSuccess, vfs_->OpenForRandomAccess(
        log_file_deleter_.path(), true, false, &raw_log_file,
        &log_file_size_));
    log_file_.reset(raw_log_file);
  }

  void CreatePool(int page_shift, int page_capacity) {
    PoolOptions options;
    options.page_shift = page_shift;
    options.page_pool_size = page_capacity;
    pool_.reset(PoolImpl::Create(options));
  }

  void WriteStorePage(StoreImpl* store, size_t page_id, const uint8_t* data) {
    ASSERT_TRUE(pool_.get() != nullptr);
    PagePool* page_pool = pool_->page_pool();
    Page* page = page_pool->AllocPage();
    ASSERT_TRUE(page != nullptr);

    ASSERT_EQ(Status::kSuccess, page_pool->AssignPageToStore(
        page, store, page_id, PagePool::kIgnorePageData));
    page->MarkDirty();
    std::memcpy(page->data(), data, 1 << kStorePageShift);
    ASSERT_EQ(Status::kSuccess, store->WritePage(page));
    page->MarkDirty(false);
    page_pool->UnassignPageFromStore(page);
    page_pool->UnpinUnassignedPage(page);
  }

  const std::string kStoreFileName = "test_free_page_list.berry";

  // 256-byte store pages have 240 bytes for free page list entres, so a page
  // has 14 entries.
  constexpr static size_t kStorePageShift = 8;  // 256-byte pages

  Vfs* vfs_;
  // Must precede UniquePtr members, because on Windows all file handles must be
  // closed before the files can be deleted.
  FileDeleter data_file_deleter_, log_file_deleter_;

  UniquePtr<PoolImpl> pool_;
  UniquePtr<BlockAccessFile> data_file_;
  size_t data_file_size_;
  UniquePtr<RandomAccessFile> log_file_;
  size_t log_file_size_;
  std::mt19937 rnd_;
};

TEST_F(FreePageListTest, Push) {
  return;
}

}  // namespace berrydb
