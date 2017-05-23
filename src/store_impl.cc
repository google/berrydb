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
    BlockAccessFile* data_file, size_t data_file_size,
    RandomAccessFile* log_file, size_t log_file_size, PagePool* page_pool,
    const StoreOptions& options) {
  void* heap_block = Allocate(sizeof(StoreImpl));
  StoreImpl* store = new (heap_block) StoreImpl(
      data_file, data_file_size, log_file, log_file_size, page_pool, options);
  DCHECK_EQ(heap_block, static_cast<void*>(store));

  return store;
}

void StoreImpl::Release() {
  this->~StoreImpl();
  void* heap_block = static_cast<void*>(this);
  Deallocate(heap_block, sizeof(StoreImpl));
}

StoreImpl::StoreImpl(
    BlockAccessFile* data_file, size_t data_file_size,
    RandomAccessFile* log_file, size_t log_file_size, PagePool* page_pool,
    const StoreOptions& options)
    : data_file_(data_file), log_file_(log_file), page_pool_(page_pool),
      page_shift_(page_pool->page_shift()) {
  DCHECK(data_file != nullptr);
  DCHECK(log_file != nullptr);
  DCHECK(page_pool != nullptr);

  // This will be used when we implement creating/loading the metadata page.
  UNUSED(options);
  UNUSED(page_pool_);
}

StoreImpl::~StoreImpl() {
  if (state_ == State::kOpen)
    Close();

  DCHECK(state_ == State::kClosed);
}

Status StoreImpl::Initialize(const StoreOptions &options) {
  UNUSED(options);

  return Status::kSuccess;
}

TransactionImpl* StoreImpl::CreateTransaction() {
  TransactionImpl* transaction = TransactionImpl::Create(this);
  transactions_.push_back(transaction);
  return transaction;
}

Status StoreImpl::Close() {
  if (state_ != State::kOpen) {
    if (state_ == State::kClosed)
      return Status::kAlreadyClosed;
    else
      return Status::kSuccess;
  }

  // We cannot transition directly into the closed state because we want to
  // abort the live transactions cleanly, assuming no I/O errors.
  state_ = State::kClosing;

  // Replace the entire transaction list so TransactionClosed() doesn't
  // invalidate our iterator.
  LinkedList<TransactionImpl> rollback_queue(std::move(transactions_));

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

  data_file_->Close();
  log_file_->Close();

  state_ = State::kClosed;
  page_pool_->pool()->StoreClosed(this);

  return result;
}

Status StoreImpl::ReadPage(Page* page) {
  DCHECK(page != nullptr);
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
  DCHECK(transaction != nullptr);
  DCHECK(transaction->IsClosed());
#if DCHECK_IS_ON()
  DCHECK_EQ(this, transaction->store());
#endif  // DCHECK_IS_ON()

  DCHECK(state_ != State::kClosed);
  if (state_ != State::kOpen)
    return;

  transactions_.erase(transaction);
}

std::string StoreImpl::LogFilePath(const std::string& store_path) {
  std::string log_path(store_path);
  log_path.append(".log", 4);
  return log_path;
}

}  // namespace berrydb
