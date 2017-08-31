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
#include "./free_page_list_format.h"
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

    UniquePtr<TransactionImpl> transaction(store->CreateTransaction());
    ASSERT_EQ(Status::kSuccess, page_pool->AssignPageToStore(
        page, store, page_id, PagePool::kIgnorePageData));
    transaction->WillModifyPage(page);
    std::memcpy(page->data(), data, 1 << kStorePageShift);
    ASSERT_EQ(Status::kSuccess, store->WritePage(page));
    transaction->PageWasPersisted(page, store->init_transaction());
    ASSERT_EQ(Status::kSuccess, transaction->Commit());
    page_pool->UnassignPageFromStore(page);
    page_pool->UnpinUnassignedPage(page);
  }

  const std::string kStoreFileName = "test_free_page_list.berry";

  static constexpr size_t kStorePageShift = 8;  // 256-byte pages

  // The first page in the data store file that can be changed.
  static constexpr size_t kBasePage = 3;
  // 256-byte store pages have 240 bytes for free page list entres, so a page
  // has 30 entries.
  static constexpr size_t kEntriesPerPage = 30;

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

constexpr size_t FreePageListTest::kStorePageShift;
constexpr size_t FreePageListTest::kBasePage;
constexpr size_t FreePageListTest::kEntriesPerPage;

TEST_F(FreePageListTest, PushPop) {
  CreatePool(kStorePageShift, 128);
  PagePool* page_pool = pool_->page_pool();

  EXPECT_EQ(0U, data_file_size_);
  UniquePtr<StoreImpl> store(StoreImpl::Create(
      data_file_.release(), data_file_size_, log_file_.release(),
      log_file_size_, page_pool, StoreOptions()));

  FreePageList free_page_list;
  TransactionImpl* alloc_transaction = store->CreateTransaction();

  for (size_t i = kBasePage; i < 128 + kBasePage; ++i)
    ASSERT_EQ(Status::kSuccess, free_page_list.Push(alloc_transaction, i));

  // Test for the empty page and entry-available-in-page case.

  for (size_t i = 128 + kBasePage - 1; i >= kBasePage; --i) {
    size_t page_id = FreePageList::kInvalidPageId;
    ASSERT_EQ(Status::kSuccess, free_page_list.Pop(
        alloc_transaction, &page_id));
    EXPECT_EQ(i, page_id);
  }

  // Test for the empty list case.

  size_t page_id = kBasePage;
  ASSERT_EQ(Status::kSuccess, free_page_list.Pop(
      alloc_transaction, &page_id));
  EXPECT_EQ(FreePageList::kInvalidPageId, page_id);
}

TEST_F(FreePageListTest, PushState) {
  CreatePool(kStorePageShift, 3);
  PagePool* page_pool = pool_->page_pool();

  EXPECT_EQ(0U, data_file_size_);
  UniquePtr<StoreImpl> store(StoreImpl::Create(
      data_file_.release(), data_file_size_, log_file_.release(),
      log_file_size_, page_pool, StoreOptions()));

  FreePageList free_page_list;
  TransactionImpl* alloc_transaction = store->CreateTransaction();

  // Test for the empty list case and space-available-in-page case.

  for (size_t i = 0; i <= kEntriesPerPage; ++i) {
    ASSERT_EQ(Status::kSuccess, free_page_list.Push(
        alloc_transaction, kBasePage + i));
    EXPECT_EQ(kBasePage, free_page_list.head_page_id());
    EXPECT_EQ(kBasePage, free_page_list.tail_page_id());

    Page* list_head_page;
    ASSERT_EQ(Status::kSuccess, page_pool->StorePage(
        store.get(), kBasePage, PagePool::kFetchPageData, &list_head_page));
    EXPECT_EQ(alloc_transaction, list_head_page->transaction());
    EXPECT_TRUE(list_head_page->is_dirty());

    uint8_t* list_head_data = list_head_page->data();
    EXPECT_EQ(
        FreePageList::kInvalidPageId,
        FreePageListFormat::NextPageId64(list_head_data));
    EXPECT_EQ(
        FreePageListFormat::kFirstEntryOffset +
            i * FreePageListFormat::kEntrySize,
        FreePageListFormat::NextEntryOffset(list_head_data));
    for (size_t j = 1; j <= i; ++j) {
      EXPECT_EQ(
          static_cast<uint64_t>(kBasePage + j),
          LoadUint64(
              list_head_data + FreePageListFormat::kFirstEntryOffset +
              (j - 1) * FreePageListFormat::kEntrySize));
    }

    // Make sure that the list didn't touch any page unnecessarily.
    EXPECT_LT(1U, page_pool->page_capacity());
    EXPECT_EQ(1U, page_pool->allocated_pages());
    EXPECT_EQ(0U, page_pool->unused_pages());

    // Evict the head page so it's not dirty anymore.
    page_pool->UnassignPageFromStore(list_head_page);
    ASSERT_FALSE(list_head_page->is_dirty());
    page_pool->UnpinUnassignedPage(list_head_page);
  }

  // Test for the page-full case.

  ASSERT_EQ(Status::kSuccess, free_page_list.Push(
      alloc_transaction, kBasePage + kEntriesPerPage + 1));
  EXPECT_EQ(kBasePage + kEntriesPerPage + 1, free_page_list.head_page_id());
  EXPECT_EQ(kBasePage, free_page_list.tail_page_id());

  Page* list_head_page;
  ASSERT_EQ(Status::kSuccess, page_pool->StorePage(
      store.get(), kBasePage + kEntriesPerPage + 1, PagePool::kFetchPageData,
      &list_head_page));
  EXPECT_EQ(alloc_transaction, list_head_page->transaction());
  EXPECT_TRUE(list_head_page->is_dirty());

  uint8_t* list_head_data = list_head_page->data();
  EXPECT_EQ(
      kBasePage, FreePageListFormat::NextPageId64(list_head_data));
  EXPECT_EQ(
      FreePageListFormat::kFirstEntryOffset,
      FreePageListFormat::NextEntryOffset(list_head_data));

  Page* list_tail_page;
  ASSERT_EQ(Status::kSuccess, page_pool->StorePage(
      store.get(), kBasePage, PagePool::kFetchPageData, &list_tail_page));
  EXPECT_EQ(store->init_transaction(), list_tail_page->transaction());
  EXPECT_FALSE(list_tail_page->is_dirty());

  uint8_t* list_tail_data = list_tail_page->data();
  EXPECT_EQ(
      FreePageList::kInvalidPageId,
      FreePageListFormat::NextPageId64(list_tail_data));
  EXPECT_EQ(
      page_pool->page_size(),
      FreePageListFormat::NextEntryOffset(list_tail_data));

  // Make sure that the list didn't touch any page unnecessarily.
  //
  // The list needed to touch the old head page to realize it's full, and the
  // new head page to build the list page data structures.
  EXPECT_LT(2U, page_pool->page_capacity());
  EXPECT_EQ(2U, page_pool->allocated_pages());
  EXPECT_EQ(0U, page_pool->unused_pages());

  // Test for the corrupted page case.

  StoreUint64(19, list_head_data + FreePageListFormat::kNextEntryOffset);
  uint8_t list_head_copy[1 << kStorePageShift];
  ASSERT_EQ(1U << kStorePageShift, page_pool->page_size());
  std::memcpy(list_head_copy, list_head_data, page_pool->page_size());
  // Evict the head page so it's not dirty anymore.
  page_pool->UnassignPageFromStore(list_head_page);
  ASSERT_FALSE(list_head_page->is_dirty());
  page_pool->UnpinUnassignedPage(list_head_page);

  EXPECT_EQ(Status::kDataCorrupted, free_page_list.Push(
      alloc_transaction, kBasePage + kEntriesPerPage + 1));

  ASSERT_EQ(Status::kSuccess, page_pool->StorePage(
      store.get(), kBasePage + kEntriesPerPage + 1, PagePool::kFetchPageData,
      &list_head_page));
  EXPECT_EQ(store->init_transaction(), list_head_page->transaction());
  EXPECT_FALSE(list_head_page->is_dirty());

  list_head_data = list_head_page->data();
  EXPECT_EQ(0, std::memcmp(
      list_head_copy, list_head_data, page_pool->page_size()));

  page_pool->UnpinStorePage(list_head_page, PagePool::kCachePage);
  page_pool->UnpinStorePage(list_tail_page, PagePool::kCachePage);
  EXPECT_EQ(Status::kSuccess, alloc_transaction->Rollback());
}

TEST_F(FreePageListTest, PopState) {
  CreatePool(kStorePageShift, 3);
  PagePool* page_pool = pool_->page_pool();

  EXPECT_EQ(0U, data_file_size_);
  UniquePtr<StoreImpl> store(StoreImpl::Create(
      data_file_.release(), data_file_size_, log_file_.release(),
      log_file_size_, page_pool, StoreOptions()));

  FreePageList free_page_list;
  TransactionImpl* alloc_transaction = store->CreateTransaction();

  // Set up the list.

  for (size_t i = 0; i <= kEntriesPerPage + 1; ++i) {
    ASSERT_EQ(Status::kSuccess, free_page_list.Push(
        alloc_transaction, kBasePage + i));
  }
  ASSERT_EQ(kBasePage + kEntriesPerPage + 1, free_page_list.head_page_id());
  ASSERT_EQ(kBasePage, free_page_list.tail_page_id());

  Page* list_head_page;
  ASSERT_EQ(Status::kSuccess, page_pool->StorePage(
      store.get(), kBasePage + kEntriesPerPage + 1, PagePool::kFetchPageData,
      &list_head_page));
  ASSERT_EQ(alloc_transaction, list_head_page->transaction());
  ASSERT_TRUE(list_head_page->is_dirty());

  Page* list_tail_page;
  ASSERT_EQ(Status::kSuccess, page_pool->StorePage(
      store.get(), kBasePage, PagePool::kFetchPageData, &list_tail_page));
  ASSERT_EQ(alloc_transaction, list_tail_page->transaction());
  ASSERT_TRUE(list_tail_page->is_dirty());

  page_pool->UnassignPageFromStore(list_head_page);
  ASSERT_FALSE(list_head_page->is_dirty());
  page_pool->UnpinUnassignedPage(list_head_page);
  page_pool->UnassignPageFromStore(list_tail_page);
  ASSERT_FALSE(list_tail_page->is_dirty());
  page_pool->UnpinUnassignedPage(list_tail_page);

  ASSERT_LT(2U, page_pool->page_capacity());
  ASSERT_EQ(2U, page_pool->allocated_pages());
  ASSERT_EQ(2U, page_pool->unused_pages());

  // Test for the empty page case.

  size_t page_id = FreePageList::kInvalidPageId;
  ASSERT_EQ(Status::kSuccess, free_page_list.Pop(
      alloc_transaction, &page_id));
  EXPECT_EQ(kBasePage + kEntriesPerPage + 1, page_id);
  EXPECT_EQ(kBasePage, free_page_list.head_page_id());
  EXPECT_EQ(kBasePage, free_page_list.tail_page_id());

  ASSERT_EQ(Status::kSuccess, page_pool->StorePage(
      store.get(), kBasePage + kEntriesPerPage + 1, PagePool::kFetchPageData,
      &list_tail_page));

  EXPECT_EQ(store->init_transaction(), list_head_page->transaction());
  EXPECT_FALSE(list_head_page->is_dirty());

  // Make sure that the list didn't touch any page unnecessarily.
  EXPECT_LT(2U, page_pool->page_capacity());
  EXPECT_EQ(2U, page_pool->allocated_pages());
  EXPECT_EQ(1U, page_pool->unused_pages());

  // Evict the tail page to make sure it doesn't get accessed anymore.
  page_pool->UnassignPageFromStore(list_tail_page);
  ASSERT_FALSE(list_tail_page->is_dirty());
  page_pool->UnpinUnassignedPage(list_tail_page);

  // Test for the entry-available-in-page case.

  for (size_t i = 0; i < kEntriesPerPage; ++i) {
    page_id = FreePageList::kInvalidPageId;
    ASSERT_EQ(Status::kSuccess, free_page_list.Pop(
        alloc_transaction, &page_id));
    EXPECT_EQ(kBasePage + kEntriesPerPage - i, page_id);
    EXPECT_EQ(kBasePage, free_page_list.head_page_id());
    EXPECT_EQ(kBasePage, free_page_list.tail_page_id());

    ASSERT_EQ(Status::kSuccess, page_pool->StorePage(
        store.get(), kBasePage, PagePool::kFetchPageData, &list_head_page));
    EXPECT_EQ(alloc_transaction, list_head_page->transaction());
    EXPECT_TRUE(list_head_page->is_dirty());

    uint8_t* list_head_data = list_head_page->data();
    EXPECT_EQ(
        FreePageList::kInvalidPageId,
        FreePageListFormat::NextPageId64(list_head_data));
    EXPECT_EQ(
        FreePageListFormat::kFirstEntryOffset +
            (kEntriesPerPage - 1 - i) * FreePageListFormat::kEntrySize,
        FreePageListFormat::NextEntryOffset(list_head_data));

    for (size_t j = 1; j <= kEntriesPerPage; ++j) {
      EXPECT_EQ(
          static_cast<uint64_t>(kBasePage + j),
          LoadUint64(
              list_head_data + FreePageListFormat::kFirstEntryOffset +
              (j - 1) * FreePageListFormat::kEntrySize));
    }

    // Make sure that the list didn't touch any page unnecessarily.
    EXPECT_LT(2U, page_pool->page_capacity());
    EXPECT_EQ(2U, page_pool->allocated_pages());
    EXPECT_EQ(1U, page_pool->unused_pages());

    // Evict the head page so it's not dirty anymore.
    page_pool->UnassignPageFromStore(list_head_page);
    ASSERT_FALSE(list_head_page->is_dirty());
    page_pool->UnpinUnassignedPage(list_head_page);
  }

  // Test for the case of an empty page leading to an empty list.

  page_id = FreePageList::kInvalidPageId;
  ASSERT_EQ(Status::kSuccess, free_page_list.Pop(
      alloc_transaction, &page_id));
  EXPECT_EQ(kBasePage, page_id);
  EXPECT_EQ(FreePageList::kInvalidPageId, free_page_list.head_page_id());
  EXPECT_EQ(FreePageList::kInvalidPageId, free_page_list.tail_page_id());

  ASSERT_EQ(Status::kSuccess, page_pool->StorePage(
      store.get(), kBasePage, PagePool::kFetchPageData, &list_head_page));
  EXPECT_EQ(store->init_transaction(), list_head_page->transaction());
  EXPECT_FALSE(list_head_page->is_dirty());

  // Make sure that the list didn't touch any page unnecessarily.
  EXPECT_LT(2U, page_pool->page_capacity());
  EXPECT_EQ(2U, page_pool->allocated_pages());
  EXPECT_EQ(1U, page_pool->unused_pages());

  // Evict the head page so it's not accessed anymore.
  page_pool->UnassignPageFromStore(list_head_page);
  ASSERT_FALSE(list_head_page->is_dirty());
  page_pool->UnpinUnassignedPage(list_head_page);

  // Test for the empty list case.

  page_id = kBasePage;
  ASSERT_EQ(Status::kSuccess, free_page_list.Pop(
      alloc_transaction, &page_id));
  EXPECT_EQ(FreePageList::kInvalidPageId, page_id);
  EXPECT_EQ(FreePageList::kInvalidPageId, free_page_list.head_page_id());
  EXPECT_EQ(FreePageList::kInvalidPageId, free_page_list.tail_page_id());

  // Make sure that the list didn't touch any page unnecessarily.
  EXPECT_LT(2U, page_pool->page_capacity());
  EXPECT_EQ(2U, page_pool->allocated_pages());
  EXPECT_EQ(2U, page_pool->unused_pages());

  EXPECT_EQ(Status::kSuccess, alloc_transaction->Rollback());
}

// TODO(pwnall): Add tests for merging.

}  // namespace berrydb
