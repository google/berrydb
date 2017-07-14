// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "./page.h"

#include <type_traits>

#include "berrydb/platform.h"
#include "./page_pool.h"
#include "./store_impl.h"
#include "./transaction_impl.h"

namespace berrydb {

Page* Page::Create(PagePool* page_pool) {
  DCHECK(page_pool != nullptr);

  size_t block_size = sizeof(Page) + page_pool->page_size();
  void* page_block = Allocate(block_size);
  Page* page = new (page_block) Page(page_pool);
  DCHECK_EQ(reinterpret_cast<void*>(page), page_block);

  // Make sure that page data is 8-byte aligned.
  DCHECK_EQ(reinterpret_cast<uintptr_t>(page->data()) & 0x07, 0U);

  return page;
}

void Page::Release(PagePool *page_pool) {
#if DCHECK_IS_ON()
  DCHECK_EQ(page_pool_, page_pool);
#endif  // DCHECK_IS_ON()

  size_t block_size = sizeof(Page) + page_pool->page_size();
  void* heap_block = reinterpret_cast<void*>(this);
  Deallocate(heap_block, block_size);
}

Page::Page(PagePool* page_pool)
    : pin_count_(1)
#if DCHECK_IS_ON()
    , page_pool_(page_pool)
#endif  // DCHECK_IS_ON()
    {
  UNUSED(page_pool);

#if DCHECK_IS_ON()
  transaction_ = nullptr;
#endif
}

Page::~Page() {
  DCHECK(transaction_ == nullptr);
}

#if DCHECK_IS_ON()
void Page::DcheckTransactionAssignmentIsValid(TransactionImpl* transaction) {
  DCHECK(transaction_ == nullptr);
  DCHECK(transaction != nullptr);
  DCHECK(transaction->store()->page_pool() == page_pool_);
}

void Page::DcheckDirtyValueIsValid(bool is_dirty) {
  // Dirty page pool entries must be assigned to non-init transactions.
  DCHECK(!is_dirty || (transaction_ != nullptr && !transaction_->IsInit()));

  // A page pool entry that just became non-dirty must have been re-assigned to
  // an init transaction, or must have been unassigned from a store.
  DCHECK(is_dirty || (transaction_ == nullptr || transaction_->IsInit()));

  // TODO(pwnall): It is currently possible for a page's dirty flag to be set to
  //               true multiple times. This is tied to whether a page is
  //               associated to a non-init transaction only when it is dirty.
  //               Re-visit the assertion below after the whole system is in
  //               place.
  // DCHECK(is_dirty != is_dirty_);
}

void Page::DcheckTransactionReassignmentIsValid(TransactionImpl* transaction) {
  DCHECK(transaction != nullptr);
  DCHECK(transaction_ != nullptr);
  DCHECK(transaction_ != transaction);
  DCHECK(transaction_->IsInit() != transaction->IsInit());
}
#endif  // DCHECK_IS_ON()

}  // namespace berrydb
