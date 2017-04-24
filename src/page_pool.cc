// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "./page_pool.h"

#include "berrydb/platform.h"
#include "./store_impl.h"

namespace berrydb {

PagePool::PagePool(size_t page_shift, size_t page_capacity)
    : page_shift_(page_shift), page_size_(1 << page_shift),
      page_capacity_(page_capacity), free_sentinel_(this), mru_sentinel_(this),
      log_sentinel_(this) {
  DCHECK_EQ(page_size_ & (page_size_ - 1), 0);
}

Page* PagePool::AllocPage() {
  if (!free_sentinel_.IsEmptyListSentinel()) {
    // The free list is used as a stack (LIFO), because the last used free page
    // has the highest chance of being in the CPU's caches.
    Page* page = free_sentinel_.next();
    free_sentinel_.RemoveFromList(page);
    page->AddPin();
    DCHECK_EQ(page->store(), nullptr);
    DCHECK(!page->is_dirty());
    return page;
  }

  if (!mru_sentinel_.IsEmptyListSentinel()) {
    Page* page = mru_sentinel_.prev();
    free_sentinel_.RemoveFromList(page);

    page->AddPin();
    if (page->is_dirty()) {
      StoreImpl* store = page->store();
      Status write_status = store->WritePage(page);
      page->MarkDirty(false);
      page->UnassignFromStore();
      if (write_status != Status::kSuccess)
        store->Close();
    } else {
      page->UnassignFromStore();
    }
    return page;
  }

  if (page_count_ < page_capacity_) {
    ++page_count_;
    Page* page = Page::Create(this);
    return page;
  }

  return nullptr;
}

Status PagePool::FetchStorePage(Page *page, PageFetchMode fetch_mode) {
  if (fetch_mode == PagePool::kFetchPageData)
    return page->store()->ReadPage(page);

  page->MarkDirty(true);
  return Status::kSuccess;
}

Status PagePool::StorePage(
    StoreImpl* store, size_t page_id, PageFetchMode fetch_mode, Page** result) {
  const auto& it = page_map_.find(std::make_pair(store, page_id));
  if (it != page_map_.end()) {
    Page* page = it->second;
    DCHECK_EQ(store, page->store());
    DCHECK_EQ(page_id, page->page_id());
    *result = page;
    return Status::kSuccess;
  }

  Page* page = AllocPage();
  if (page == nullptr)
    return Status::kPoolFull;

  page->AssignToStore(store, page_id);
  Status fetch_status = FetchStorePage(page, fetch_mode);
  if (fetch_status == Status::kSuccess) {
    page_map_[std::make_pair(store, page_id)] = page;
    *result = page;
    return Status::kSuccess;
  }

  page->UnassignFromStore();
  page->RemovePin();
  DCHECK(page->IsUnpinned());
  free_sentinel_.InsertPageAfter(page, &free_sentinel_);
  return fetch_status;
}

void PagePool::UnpinStorePage(Page* page) {
  DCHECK_EQ(this, page->page_pool());
  DCHECK(page->store() != nullptr);

  page->RemovePin();
  if (page->IsUnpinned())
    mru_sentinel_.InsertPageAfter(page, &mru_sentinel_);
}

}  // namespace berrydb
