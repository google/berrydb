// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "./transaction_impl.h"

#include "berrydb/status.h"
#include "./page_pool.h"
#include "./store_impl.h"

// TODO(pwnall): Remove this once we don't need to DCHECK a Status value.
#include "berrydb/ostream_ops.h"

namespace berrydb {

static_assert(std::is_standard_layout<TransactionImpl>::value,
    "TransactionImpl must be a standard layout type so its public API can be "
    "exposed cheaply");

TransactionImpl* TransactionImpl::Create(StoreImpl* store) {
  void* heap_block = Allocate(sizeof(TransactionImpl));
  TransactionImpl* transaction = new (heap_block) TransactionImpl(store);
  DCHECK_EQ(heap_block, static_cast<void*>(transaction));
  return transaction;
}

void TransactionImpl::Release() {
  this->~TransactionImpl();
  void* heap_block = static_cast<void*>(this);
  Deallocate(heap_block, sizeof(TransactionImpl));
}

TransactionImpl::TransactionImpl(StoreImpl* store)
    : store_(store)
#if DCHECK_IS_ON()
    , is_init_(false)
#endif  // DCHECK_IS_ON()
    {
  DCHECK(store != nullptr);
}

TransactionImpl::TransactionImpl(StoreImpl* store, bool is_init)
    : store_(store)
#if DCHECK_IS_ON()
    , is_init_(true)
#endif  // DCHECK_IS_ON()
    {
  DCHECK(store != nullptr);
  DCHECK(is_init == true);
  UNUSED(is_init);
}

TransactionImpl::~TransactionImpl() {
  if (!is_closed_)
    Rollback();
}


#if DCHECK_IS_ON()
void TransactionImpl::DcheckPageBelongsToTransaction(Page* page) {
  DCHECK(page != nullptr);
  DCHECK_EQ(page->transaction(), this);
  DCHECK_EQ(store_->page_pool(), page->page_pool());
}

bool TransactionImpl::IsInit() const noexcept {
  return store_->init_transaction() == this;
}
#endif  // DCHECK_IS_ON()

Status TransactionImpl::Get(Space* space, string_view key, string_view* value) {
  if (is_closed_)
    return Status::kAlreadyClosed;

  UNUSED(space);
  UNUSED(key);
  UNUSED(value);
  return Status::kIoError;
}

Status TransactionImpl::Put(Space* space, string_view key, string_view value) {
  if (is_closed_)
    return Status::kAlreadyClosed;

  UNUSED(space);
  UNUSED(key);
  UNUSED(value);
  return Status::kIoError;
}

Status TransactionImpl::Delete(Space* space, string_view key) {
  if (is_closed_)
    return Status::kAlreadyClosed;

  UNUSED(space);
  UNUSED(key);
  return Status::kIoError;
}

Status TransactionImpl::Close() {
  DCHECK(!is_closed_);

  is_closed_ = true;

  // Unassign the pages that are assigned to this transaction.

  PagePool* page_pool = store_->page_pool();
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
  DCHECK(this != store_->init_transaction());

  if (is_closed_)
    return Status::kAlreadyClosed;

  // Write the pages modified by this transaction.
  //
  // This must be a non-init transaction, because only non-init transactions can
  // commit. So, all the pages assigned to this transaction must be pages that
  // have been modified by it.

  PagePool* page_pool = store_->page_pool();
  page_pool->PinTransactionPages(&pool_pages_);

  TransactionImpl* init_transaction = store_->init_transaction();

  // We cannot use C++11's range-based for loop because the iterator would get
  // invalidated when we remove the page it's pointing to from the list.
  for (auto it = pool_pages_.begin(); it != pool_pages_.end(); ) {
    Page* page = *it;
    ++it;

    // TODO(pwnall): Write REDO records for the pages to the log instead. The
    //               log write status handling code will remain the same.
    Status status = store_->WritePage(page);

    // TODO(pwnall): Handle errors, once we have logging in place.
    DCHECK_EQ(status, Status::kSuccess);
    UNUSED(status);

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
  if (is_closed_)
    return Status::kAlreadyClosed;

  return Close();
}

Status TransactionImpl::CreateSpace(
    CatalogImpl* catalog, string_view name, SpaceImpl** result) {
  if (is_closed_)
    return Status::kAlreadyClosed;

  UNUSED(catalog);
  UNUSED(name);
  UNUSED(result);
  return Status::kIoError;

}

Status TransactionImpl::CreateCatalog(
    CatalogImpl* catalog, string_view name, CatalogImpl** result) {

  if (is_closed_)
    return Status::kAlreadyClosed;
  UNUSED(catalog);
  UNUSED(name);
  UNUSED(result);
  return Status::kIoError;
}

Status TransactionImpl::Delete(CatalogImpl* catalog, string_view name) {
  if (is_closed_)
    return Status::kAlreadyClosed;

  UNUSED(catalog);
  UNUSED(name);
  return Status::kIoError;
}

}  // namespace berrydb
