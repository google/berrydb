// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "./page.h"

#include <type_traits>

#include "berrydb/platform.h"
#include "./page_pool.h"
#include "./store_impl.h"
#include "./transaction_impl.h"
#include "./util/checks.h"

namespace berrydb {

Page* Page::Create(PagePool* page_pool) {
  BERRYDB_ASSUME(page_pool != nullptr);

  const size_t block_size = sizeof(Page) + page_pool->page_size();
  void* const page_block = Allocate(block_size);
  Page* const page = new (page_block) Page(page_pool);
  BERRYDB_ASSUME_EQ(reinterpret_cast<void*>(page), page_block);

  // Make sure that page data is 8-byte aligned.
  BERRYDB_ASSUME_EQ(reinterpret_cast<uintptr_t>(page->buffer()) & 0x07, 0U);

  return page;
}

void Page::Release(PagePool *page_pool) {
#if BERRYDB_CHECK_IS_ON()
  BERRYDB_CHECK_EQ(page_pool_, page_pool);
#endif  // BERRYDB_CHECK_IS_ON()

  const size_t block_size = sizeof(Page) + page_pool->page_size();
  void* const heap_block = reinterpret_cast<void*>(this);
  Deallocate(heap_block, block_size);
}

Page::Page(MAYBE_UNUSED PagePool* page_pool)
    : pin_count_(1)
#if BERRYDB_CHECK_IS_ON()
    , page_pool_(page_pool)
#endif  // BERRYDB_CHECK_IS_ON()
    {

#if BERRYDB_CHECK_IS_ON()
  transaction_ = nullptr;
#endif  // BERRYDB_CHECK_IS_ON()
}

Page::~Page() {
  BERRYDB_ASSUME(transaction_ == nullptr);
}

#if BERRYDB_CHECK_IS_ON()

void Page::CheckTransactionAssignmentIsValid(
    TransactionImpl* transaction) noexcept {
  BERRYDB_CHECK(transaction_ == nullptr);
  BERRYDB_CHECK(transaction != nullptr);
  BERRYDB_CHECK_EQ(transaction->store()->page_pool(), page_pool_);
}

void Page::CheckNewDirtyValueIsValid(bool is_dirty) noexcept {
  // Dirty page pool entries must be assigned to non-init transactions.
  BERRYDB_CHECK(
      !is_dirty || (transaction_ != nullptr && !transaction_->IsInit()));

  // A page pool entry that just became non-dirty must have been re-assigned to
  // an init transaction, or must have been unassigned from a store.
  BERRYDB_CHECK(
      is_dirty || (transaction_ == nullptr || transaction_->IsInit()));

  // TODO(pwnall): It is currently possible for a page's dirty flag to be set to
  //               true multiple times. This is tied to whether a page is
  //               associated to a non-init transaction only when it is dirty.
  //               Re-visit the CHECK below after the whole system is in place.
  // BERRYDB_CHECK_NE(is_dirty, is_dirty_);
}

void Page::CheckTransactionReassignmentIsValid(
    TransactionImpl* transaction) noexcept {
  BERRYDB_CHECK(transaction != nullptr);
  BERRYDB_CHECK(transaction_ != nullptr);
  BERRYDB_CHECK_NE(transaction_, transaction);
  BERRYDB_CHECK_NE(transaction_->IsInit(), transaction->IsInit());
}

void Page::CheckPageSizeMatches(size_t page_size) const noexcept {
  BERRYDB_CHECK_EQ(page_size, page_pool_->page_size());
}

#endif  // BERRYDB_CHECK_IS_ON()

}  // namespace berrydb
