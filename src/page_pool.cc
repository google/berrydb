// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "./page_pool.h"

#include <algorithm>

#include "./store_impl.h"
#include "./util/checks.h"
#include "./util/span_util.h"

namespace berrydb {

PagePool::PagePool(PoolImpl* pool, size_t page_shift, size_t page_capacity)
    : page_shift_(page_shift), page_size_(static_cast<size_t>(1) << page_shift),
      page_capacity_(page_capacity), pool_(pool), free_list_(), lru_list_(),
      log_list_() {
  BERRYDB_ASSUME(pool != nullptr);
  // The page size should be a power of two.
  BERRYDB_ASSUME_EQ(page_size_ & (page_size_ - 1), 0U);
}

PagePool::~PagePool() {
  BERRYDB_ASSUME_EQ(pinned_pages(), 0U);

  // We cannot use C++11's range-based for loop because the iterator would get
  // invalidated if we release the page it's pointing to.

  for (auto it = free_list_.begin(); it != free_list_.end(); ) {
    Page* page = *it;
    ++it;
    page->Release(this);
  }

  // The LRU list should be empty, unless we crash-close.

  for (auto it = lru_list_.begin(); it != lru_list_.end(); ) {
    Page* page = *it;
    ++it;
    page->Release(this);
  }
}

void PagePool::UnpinUnassignedPage(Page* page) {
  BERRYDB_ASSUME(page != nullptr);
#if BERRYDB_CHECK_IS_ON()
  BERRYDB_CHECK_EQ(page->page_pool(), this);
#endif  // BERRYDB_CHECK_IS_ON()
  BERRYDB_ASSUME(page->transaction() == nullptr);

  page->RemovePin();
  if (page->IsUnpinned())
    free_list_.push_back(page);
}

void PagePool::UnassignPageFromStore(Page* page) {
  BERRYDB_ASSUME(page != nullptr);
  BERRYDB_ASSUME(page->transaction() != nullptr);
  BERRYDB_ASSUME(page->transaction()->store() != nullptr);
#if BERRYDB_CHECK_IS_ON()
  BERRYDB_CHECK_EQ(page->page_pool(), this);
#endif  // BERRYDB_CHECK_IS_ON()

  TransactionImpl* transaction = page->transaction();
  StoreImpl* store = transaction->store();
  BERRYDB_ASSUME_EQ(1U,
                    page_map_.count(std::make_pair(store, page->page_id())));
  page_map_.erase(std::make_pair(store, page->page_id()));
  if (page->is_dirty()) {
    Status write_status = store->WritePage(page);
    transaction->UnassignPersistedPage(page);
    if (UNLIKELY(write_status != Status::kSuccess))
      store->Close();
  } else {
    transaction->UnassignPage(page);
  }
}

Page* PagePool::AllocPage() {
  if (!free_list_.empty()) {
    // The free list is used as a stack (LIFO), because the last used free page
    // has the highest chance of being in the CPU's caches.
    Page* page = free_list_.front();
    free_list_.pop_front();
    page->AddPin();
    BERRYDB_ASSUME(page->transaction() == nullptr);
    BERRYDB_ASSUME(!page->is_dirty());
    return page;
  }

  if (page_count_ < page_capacity_) {
    ++page_count_;
    Page* page = Page::Create(this);
    return page;
  }

  if (!lru_list_.empty()) {
    Page* page = lru_list_.front();
    page->AddPin();
    lru_list_.pop_front();
    UnassignPageFromStore(page);
    return page;
  }

  return nullptr;
}

Status PagePool::FetchStorePage(Page *page, PageFetchMode fetch_mode) {
  BERRYDB_ASSUME(page != nullptr);
  BERRYDB_ASSUME(page->transaction() != nullptr);
  BERRYDB_ASSUME(page->transaction()->store() != nullptr);
#if BERRYDB_CHECK_IS_ON()
  BERRYDB_CHECK_EQ(page->page_pool(), this);
#endif  // BERRYDB_CHECK_IS_ON()

  if (fetch_mode == PagePool::kFetchPageData)
    return page->transaction()->store()->ReadPage(page);

  // Technically, the page should be marked dirty here, to reflect the fact that
  // the in-memory data does not match the on-disk page content. However,
  // fetch_mode must be PagePool::kIgnorePageData, so the caller will mark the
  // page dirty anyway.

#if BERRYDB_CHECK_IS_ON()
  span<uint8_t> page_data = page->mutable_data(page_size_);
  // Fill the page with recognizable garbage (as opposed to random garbage), to
  // make it easier to spot code that uses uninitialized page data.
  FillSpan(page_data, 0xCD);
#endif  // BERRYDB_CHECK_IS_ON()

  return Status::kSuccess;
}

Status PagePool::AssignPageToStore(
    Page* page, StoreImpl* store, size_t page_id, PageFetchMode fetch_mode) {
  BERRYDB_ASSUME(page != nullptr);
  BERRYDB_ASSUME(store != nullptr);
  BERRYDB_ASSUME(page->transaction() == nullptr);
#if BERRYDB_CHECK_IS_ON()
  BERRYDB_CHECK_EQ(page->page_pool(), this);
#endif  // BERRYDB_CHECK_IS_ON()

  TransactionImpl* transaction = store->init_transaction();
  transaction->AssignPage(page, page_id);
  Status fetch_status = FetchStorePage(page, fetch_mode);
  if (LIKELY(fetch_status == Status::kSuccess)) {
    page_map_[std::make_pair(store, page_id)] = page;
    return Status::kSuccess;
  }

  transaction->UnassignPage(page);
  return fetch_status;
}

void PagePool::PinStorePage(Page* page) {
  BERRYDB_ASSUME(page != nullptr);
  BERRYDB_ASSUME(page->transaction() != nullptr);
#if BERRYDB_CHECK_IS_ON()
  BERRYDB_CHECK_EQ(page->page_pool(), this);
#endif  // BERRYDB_CHECK_IS_ON()

  // If the page is already pinned, it is not contained in any list. If the page
  // has no pins, it must be in the LRU list.
  if (page->IsUnpinned())
    lru_list_.erase(page);
  page->AddPin();
}

void PagePool::PinTransactionPages(
    LinkedList<Page, Page::TransactionLinkedListBridge> *page_list) {
  BERRYDB_ASSUME(page_list != nullptr);

  for (Page* page : *page_list) {
    BERRYDB_ASSUME(page->transaction() != nullptr);
#if BERRYDB_CHECK_IS_ON()
    BERRYDB_CHECK_EQ(page->page_pool(), this);
#endif  // BERRYDB_CHECK_IS_ON()
    PinStorePage(page);
  }
}

std::tuple<Status, Page*> PagePool::StorePage(StoreImpl* store, size_t page_id,
                                              PageFetchMode fetch_mode) {
  BERRYDB_ASSUME(store != nullptr);

  const auto& it = page_map_.find(std::make_pair(store, page_id));
  if (it != page_map_.end()) {
    Page* page = it->second;
    BERRYDB_ASSUME(page != nullptr);
    BERRYDB_ASSUME_EQ(store, page->transaction()->store());
    BERRYDB_ASSUME_EQ(page_id, page->page_id());
#if BERRYDB_CHECK_IS_ON()
    BERRYDB_CHECK_EQ(page->page_pool(), this);
#endif  // BERRYDB_CHECK_IS_ON()

    // The page can either be pinned (by another transaction/cursor) or unpinned
    // and waiting in the LRU list. The check in PinStorePage() is needed for
    // correctness.
    PinStorePage(page);
    return {Status::kSuccess, page};
  }

  Page* page = AllocPage();
  if (page == nullptr)
    return {Status::kPoolFull, nullptr};
#if BERRYDB_CHECK_IS_ON()
  BERRYDB_CHECK_EQ(page->page_pool(), this);
#endif  // BERRYDB_CHECK_IS_ON()

  Status status = AssignPageToStore(page, store, page_id, fetch_mode);
  if (LIKELY(status == Status::kSuccess))
    return {status, page};

  // Calling UnpinUnassignedPage will perform an extra check compared to
  // inlining the code, because the inlined version would know that the page
  // is unpinned. We favor code size over speed here because this is an error
  // condition.
  UnpinUnassignedPage(page);
  BERRYDB_ASSUME(page->IsUnpinned());
  return {status, nullptr};
}

}  // namespace berrydb
