// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "./page_pool.h"

#include <cstring>

#include "berrydb/platform.h"
#include "./store_impl.h"

namespace berrydb {

PagePool::PagePool(PoolImpl* pool, size_t page_shift, size_t page_capacity)
    : page_shift_(page_shift), page_size_(1 << page_shift),
      page_capacity_(page_capacity), pool_(pool), free_list_(), lru_list_(),
      log_list_() {
  // The page size should be a power of two.
  DCHECK_EQ(page_size_ & (page_size_ - 1), 0);
}

PagePool::~PagePool() {
  DCHECK_EQ(0, pinned_pages());

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

void PagePool::UnpinStorePage(Page* page) {
  DCHECK(page != nullptr);
  DCHECK(page->store() != nullptr);
#if DCHECK_IS_ON()
  DCHECK_EQ(this, page->page_pool());
#endif  // DCHECK_IS_ON()

  page->RemovePin();
  if (page->IsUnpinned())
    lru_list_.push_back(page);
}

void PagePool::UnpinAndWriteStorePage(Page* page) {
  DCHECK(page != nullptr);
  DCHECK(page->is_dirty());
  DCHECK(page->store() != nullptr);
#if DCHECK_IS_ON()
  DCHECK_EQ(this, page->page_pool());
#endif  // DCHECK_IS_ON()

  // TODO(pwnall): After all the code is written, figure out if there's a more
  //               compact version of this code, depending on where it is used.

  page->RemovePin();
  if (!page->IsUnpinned())
    return;

  Status write_status = page->store()->WritePage(page);
  page->MarkDirty(false);
  if (write_status != Status::kSuccess) {
    // TODO(pwnall): Figure out the correct strategy for unassigning here.
    StoreImpl* store = page->store();
    page->UnassignFromStore();
    store->PageUnassigned(page);
    free_list_.push_back(page);
    store->Close();
    return;
  }

  lru_list_.push_back(page);
}

void PagePool::UnpinUnassignedPage(Page* page) {
  DCHECK(page != nullptr);
#if DCHECK_IS_ON()
  DCHECK_EQ(this, page->page_pool());
#endif  // DCHECK_IS_ON()
  DCHECK(page->store() == nullptr);

  page->RemovePin();
  if (page->IsUnpinned())
    free_list_.push_back(page);
}

void PagePool::UnassignPageFromStore(Page* page) {
  DCHECK(page != nullptr);
  DCHECK(page->store() != nullptr);
#if DCHECK_IS_ON()
  DCHECK_EQ(this, page->page_pool());
#endif  // DCHECK_IS_ON()

  StoreImpl* store = page->store();
  DCHECK_EQ(1U, page_map_.count(std::make_pair(store, page->page_id())));
  page_map_.erase(std::make_pair(store, page->page_id()));
  if (page->is_dirty()) {
    Status write_status = store->WritePage(page);
    page->MarkDirty(false);

    page->UnassignFromStore();
    store->PageUnassigned(page);
    if (write_status != Status::kSuccess)
      store->Close();
  } else {
    page->UnassignFromStore();
    store->PageUnassigned(page);
  }
}

Page* PagePool::AllocPage() {
  if (!free_list_.empty()) {
    // The free list is used as a stack (LIFO), because the last used free page
    // has the highest chance of being in the CPU's caches.
    Page* page = free_list_.front();
    free_list_.pop_front();
    page->AddPin();
    DCHECK(page->store() == nullptr);
    DCHECK(!page->is_dirty());
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
  DCHECK(page != nullptr);
  DCHECK(page->store() != nullptr);
#if DCHECK_IS_ON()
  DCHECK_EQ(this, page->page_pool());
#endif  // DCHECK_IS_ON()

  if (fetch_mode == PagePool::kFetchPageData)
    return page->store()->ReadPage(page);

  // Technically, the page should be marked dirty here, to reflect the fact that
  // the in-memory data does not match the on-disk page content. However,
  // fetch_mode must be PagePool::kIgnorePageData, so the caller will mark the
  // page dirty anyway.

#if DCHECK_IS_ON()
  // Fill the page with recognizable garbage (as opposed to random garbage), to
  // make it easier to spot code that uses uninitialized page data.
  std::memset(page->data(), 0xCD, page_size_);
#endif  // DCHECK_IS_ON()

  return Status::kSuccess;
}

Status PagePool::AssignPageToStore(
    Page* page, StoreImpl* store, size_t page_id, PageFetchMode fetch_mode) {
  DCHECK(page != nullptr);
  DCHECK(store != nullptr);
  DCHECK(page->store() == nullptr);
#if DCHECK_IS_ON()
  DCHECK_EQ(this, page->page_pool());
#endif  // DCHECK_IS_ON()

  page->AssignToStore(store, page_id);
  store->PageAssigned(page);
  Status fetch_status = FetchStorePage(page, fetch_mode);
  if (fetch_status == Status::kSuccess) {
    page_map_[std::make_pair(store, page_id)] = page;
    return Status::kSuccess;
  }

  page->UnassignFromStore();
  store->PageUnassigned(page);
  return fetch_status;
}

void PagePool::PinStorePage(Page* page) {
  DCHECK(page != nullptr);
  DCHECK(page->store() != nullptr);
#if DCHECK_IS_ON()
  DCHECK_EQ(this, page->page_pool());
#endif  // DCHECK_IS_ON()

  // If the page is already pinned, it is not contained in any list. If the page
  // has no pins, it must be in the LRU list.
  if (page->IsUnpinned())
    lru_list_.erase(page);
  page->AddPin();
}

void PagePool::PinStorePages(
    LinkedList<Page, Page::StoreLinkedListBridge> *page_list) {
  DCHECK(page_list != nullptr);

  for (Page* page : *page_list) {
    DCHECK(page->store() != nullptr);
#if DCHECK_IS_ON()
    DCHECK_EQ(this, page->page_pool());
#endif  // DCHECK_IS_ON()
    PinStorePage(page);
  }
}

Status PagePool::StorePage(
    StoreImpl* store, size_t page_id, PageFetchMode fetch_mode, Page** result) {
  DCHECK(store != nullptr);

  const auto& it = page_map_.find(std::make_pair(store, page_id));
  if (it != page_map_.end()) {
    Page* page = it->second;
    DCHECK_EQ(store, page->store());
    DCHECK_EQ(page_id, page->page_id());
#if DCHECK_IS_ON()
    DCHECK_EQ(this, page->page_pool());
#endif  // DCHECK_IS_ON()

    // The page can either be pinned (by another transaction/cursor) or unpinned
    // and waiting in the LRU list. The check in PinStorePage() is needed for
    // correctness.
    PinStorePage(page);
    *result = page;
    return Status::kSuccess;
  }

  Page* page = AllocPage();
  if (page == nullptr)
    return Status::kPoolFull;
#if DCHECK_IS_ON()
  DCHECK_EQ(this, page->page_pool());
#endif  // DCHECK_IS_ON()

  Status status = AssignPageToStore(page, store, page_id, fetch_mode);
  if (status == Status::kSuccess) {
    *result = page;
  } else {
    // Calling UnpinUnassignedPage will perform an extra check compared to
    // inlining the code, because the inlined version would know that the page
    // is unpinned. We favor code size over speed here because this is an error
    // condition.
    UnpinUnassignedPage(page);
    DCHECK(page->IsUnpinned());
  }
  return status;
}

}  // namespace berrydb
