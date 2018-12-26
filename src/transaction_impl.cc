// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "./transaction_impl.h"

#include "berrydb/platform.h"
#include "berrydb/status.h"
#include "./page_pool.h"
#include "./store_impl.h"
#include "./util/checks.h"

// TODO(pwnall): Remove this once we don't need to CHECK a Status value.
#include "berrydb/ostream_ops.h"

namespace berrydb {

static_assert(std::is_standard_layout<TransactionImpl>::value,
    "TransactionImpl must be a standard layout type so its public API can be "
    "exposed cheaply");

TransactionImpl* TransactionImpl::Create(StoreImpl* store) {
  void* const heap_block = Allocate(sizeof(TransactionImpl));
  TransactionImpl* const transaction = new (heap_block) TransactionImpl(store);
  BERRYDB_ASSUME_EQ(heap_block, static_cast<void*>(transaction));
  return transaction;
}

void TransactionImpl::Release() {
  this->~TransactionImpl();
  void* const heap_block = static_cast<void*>(this);
  Deallocate(heap_block, sizeof(TransactionImpl));
}

TransactionImpl::TransactionImpl(StoreImpl* store)
    : store_(store)
#if BERRYDB_CHECK_IS_ON()
    , is_init_(false)
#endif  // BERRYDB_CHECK_IS_ON()
    {
  BERRYDB_ASSUME(store != nullptr);
}

TransactionImpl::TransactionImpl(StoreImpl* store, MAYBE_UNUSED bool is_init)
    : store_(store)
#if BERRYDB_CHECK_IS_ON()
    , is_init_(true)
#endif  // BERRYDB_CHECK_IS_ON()
    {
  BERRYDB_ASSUME(store != nullptr);
  BERRYDB_ASSUME_EQ(is_init, true);
}

TransactionImpl::~TransactionImpl() {
  if (!is_closed_)
    Rollback();
}


#if BERRYDB_CHECK_IS_ON()
void TransactionImpl::CheckPageBelongsToTransaction(Page* page) {
  BERRYDB_ASSUME(page != nullptr);
  BERRYDB_ASSUME_EQ(page->transaction(), this);
  BERRYDB_ASSUME_EQ(store_->page_pool(), page->page_pool());
}

bool TransactionImpl::IsInit() const noexcept {
  return store_->init_transaction() == this;
}
#endif  // BERRYDB_CHECK_IS_ON()

std::tuple<Status, span<const uint8_t>> TransactionImpl::Get(
    MAYBE_UNUSED SpaceImpl* space, MAYBE_UNUSED span<const uint8_t> key) {
  if (UNLIKELY(is_closed_))
    return {Status::kAlreadyClosed, span<const uint8_t>()};

  return {Status::kIoError, span<const uint8_t>()};
}

Status TransactionImpl::Put(MAYBE_UNUSED SpaceImpl* space,
                            MAYBE_UNUSED span<const uint8_t> key,
                            MAYBE_UNUSED span<const uint8_t> value) {
  if (UNLIKELY(is_closed_))
    return Status::kAlreadyClosed;

  return Status::kIoError;
}

Status TransactionImpl::Delete(MAYBE_UNUSED SpaceImpl* space,
                               MAYBE_UNUSED span<const uint8_t> key) {
  if (UNLIKELY(is_closed_))
    return Status::kAlreadyClosed;

  return Status::kIoError;
}

Status TransactionImpl::Close() {
  BERRYDB_ASSUME(!is_closed_);

  is_closed_ = true;

  // Unassign the pages that are assigned to this transaction.

  PagePool* const page_pool = store_->page_pool();
  page_pool->PinTransactionPages(&pool_pages_);

  // We cannot use C++11's range-based for loop because the iterator would get
  // invalidated when we remove the page it's pointing to from the list.
  for (auto it = pool_pages_.begin(); it != pool_pages_.end(); ) {
    Page* page = *it;
    ++it;
    page_pool->UnassignPageFromStore(page);
    page_pool->UnpinUnassignedPage(page);
  }

  store_->TransactionClosed(this);
  return Status::kSuccess;
}

Status TransactionImpl::Commit() {
  BERRYDB_ASSUME_NE(this, store_->init_transaction());

  if (UNLIKELY(is_closed_))
    return Status::kAlreadyClosed;

  // Write the pages modified by this transaction.
  //
  // This must be a non-init transaction, because only non-init transactions can
  // commit. So, all the pages assigned to this transaction must be pages that
  // have been modified by it.

  PagePool* const page_pool = store_->page_pool();
  page_pool->PinTransactionPages(&pool_pages_);

  TransactionImpl* const init_transaction = store_->init_transaction();

  // We cannot use C++11's range-based for loop because the iterator would get
  // invalidated when we remove the page it's pointing to from the list.
  for (auto it = pool_pages_.begin(); it != pool_pages_.end(); ) {
    Page* page = *it;
    ++it;

    // TODO(pwnall): Write REDO records for the pages to the log instead. The
    //               log write status handling code will remain the same.
    MAYBE_UNUSED Status status = store_->WritePage(page);

    // TODO(pwnall): Handle errors, once we have logging in place.
    BERRYDB_ASSUME_EQ(status, Status::kSuccess);

    PageWasPersisted(page, init_transaction);
    page_pool->UnpinStorePage(page);
  }

  // TODO(pwnall): Instead of moving the pages between transaction lists one by
  //               one, we could insert the committed transaction list into the
  //               init transaction list in O(1).

  is_committed_ = true;
  return Close();
}

Status TransactionImpl::Rollback() {
  if (UNLIKELY(is_closed_))
    return Status::kAlreadyClosed;

  return Close();
}

std::tuple<Status, SpaceImpl*> TransactionImpl::CreateSpace(
    MAYBE_UNUSED CatalogImpl* catalog, MAYBE_UNUSED span<const uint8_t> name) {
  if (UNLIKELY(is_closed_))
    return {Status::kAlreadyClosed, nullptr};

  return {Status::kIoError, nullptr};
}

std::tuple<Status, CatalogImpl*> TransactionImpl::CreateCatalog(
    MAYBE_UNUSED CatalogImpl* catalog, MAYBE_UNUSED span<const uint8_t> name) {
  if (UNLIKELY(is_closed_))
    return {Status::kAlreadyClosed, nullptr};

  return {Status::kIoError, nullptr};
}

Status TransactionImpl::Delete(MAYBE_UNUSED CatalogImpl* catalog,
                               MAYBE_UNUSED span<const uint8_t> name) {
  if (UNLIKELY(is_closed_))
    return Status::kAlreadyClosed;

  return Status::kIoError;
}

}  // namespace berrydb
