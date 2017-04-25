// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "./store_impl.h"

#include "berrydb/options.h"
#include "berrydb/vfs.h"
#include "./pool_impl.h"
#include "./transaction_impl.h"

namespace berrydb {

static_assert(std::is_standard_layout<StoreImpl>::value,
    "StoreImpl must be a standard layout type so its public API can be "
    "exposed cheaply");

StoreImpl* StoreImpl::Create(
      BlockAccessFile* data_file, PagePool* page_pool,
      const StoreOptions& options) {
  void* heap_block = Allocate(sizeof(StoreImpl));
  StoreImpl* store = new (heap_block) StoreImpl(data_file, page_pool, options);
  DCHECK_EQ(heap_block, static_cast<void*>(store));
  return store;
}

void StoreImpl::Release() {
  this->~StoreImpl();
  void* heap_block = static_cast<void*>(this);
  Deallocate(heap_block, sizeof(StoreImpl));
}

StoreImpl::StoreImpl(
    BlockAccessFile* data_file, PagePool* page_pool,
    const StoreOptions& options)
    : data_file_(data_file), page_pool_(page_pool),
      page_shift_(page_pool->page_shift()) {
  DCHECK(data_file != nullptr);
  DCHECK(page_pool != nullptr);
}

StoreImpl::~StoreImpl() {
  if (!is_closed_)
    Close();
}

TransactionImpl* StoreImpl::CreateTransaction() {
  TransactionImpl* transaction = TransactionImpl::Create(this);
  transactions_.insert(transaction);
  return transaction;
}

Status StoreImpl::Close() {
  if (is_closed_)
    return Status::kAlreadyClosed;

#if DCHECK_IS_ON()
  is_closing_ = true;
#endif  // DCHECK_IS_ON()

  // Replace the entire transaction list so TransactionClosed() doesn't
  // invalidate our iterator.
  TransactionSet rollback_queue;
  rollback_queue.swap(transactions_);

  Status result = Status::kSuccess;
  for (TransactionImpl* transaction : rollback_queue) {
    Status rollback_status = transaction->Rollback();

    // Report the first non-success status encountered while rolling back the
    // running transactions. If an I/O error occurs, the first transaction will
    // rollback with kIoError, but the following transactions will rollback with
    // kAlreadyClosed. It's nice to return kIoError in this case.
    if (rollback_status != Status::kSuccess && result == Status::kSuccess)
      result = rollback_status;
  }

  // The closed_ flag cannot be set earlier, because we want to abort the live
  // transactions cleanly, if there are no I/O errors.
  is_closed_ = true;
  return result;
}

Status StoreImpl::ReadPage(Page* page) {
  DCHECK_EQ(this, page->store());
  DCHECK(!page->is_dirty());

  size_t file_offset = page->page_id() << page_shift_;
  size_t page_size = 1 << page_shift_;
  return data_file_->Read(file_offset, page_size, page->data());
}

Status StoreImpl::WritePage(Page* page) {
  DCHECK_EQ(this, page->store());

  size_t file_offset = page->page_id() << page_shift_;
  size_t page_size = 1 << page_shift_;
  return data_file_->Write(page->data(), file_offset, page_size);
}

void StoreImpl::TransactionClosed(TransactionImpl* transaction) {
  DCHECK_EQ(this, transaction->store());
  DCHECK(transaction->IsClosed());

  DCHECK(is_closing_ || transactions_.count(transaction) == 1);
  transactions_.erase(transaction);
}

}  // namespace berrydb
