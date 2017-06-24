// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "./store_impl.h"

#include <cstring>

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

  page_pool->pool()->StoreCreated(store);
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
      init_transaction_(this, true), header_(
          page_pool->page_shift(), data_file_size >> page_pool->page_shift()) {
  DCHECK(data_file != nullptr);
  DCHECK(log_file != nullptr);
  DCHECK(page_pool != nullptr);

  // This will be used when we implement creating/loading the metadata page.
  UNUSED(options);

  // This will be used when we implement log recovery.
  UNUSED(log_file_size);
}

StoreImpl::~StoreImpl() {
  if (state_ == State::kOpen)
    Close();

  DCHECK(state_ == State::kClosed);
}

Status StoreImpl::Initialize(const StoreOptions &options) {
  // TODO(pwnall): Check the log and attempt recovery.

  if (options.create_if_missing && header_.page_count < 3) {
    Status status = Bootstrap();
    if (status != Status::kSuccess)
      return status;
  }

  return Status::kSuccess;
}

Status StoreImpl::Bootstrap() {
  Page* header_page;
  Status fetch_status = page_pool_->StorePage(
      this, 0, PagePool::kIgnorePageData, &header_page);
  if (fetch_status != Status::kSuccess)
    return fetch_status;

  header_page->MarkDirty();
  uint8_t* header_data = header_page->data();
  std::memset(header_data, 0, 1 << header_.page_shift);
  header_.free_list_head_page = 1;
  header_.page_count = 3;
  // header.page_shift is already set correctly by the constructor.
  header_.Serialize(header_data);
  page_pool_->UnpinAndWriteStorePage(header_page);

  Page* free_list_head_page;
  fetch_status = page_pool_->StorePage(
      this, 1, PagePool::kIgnorePageData, &free_list_head_page);
  if (fetch_status != Status::kSuccess)
    return fetch_status;

  free_list_head_page->MarkDirty();
  std::memset(free_list_head_page->data(), 0, 1 << header_.page_shift);
  // TODO(pwnall): Bootstrap the free page list here.
  page_pool_->UnpinAndWriteStorePage(free_list_head_page);

  Page* root_catalog_page;
  fetch_status = page_pool_->StorePage(
      this, 2, PagePool::kIgnorePageData, &root_catalog_page);
  if (fetch_status != Status::kSuccess)
    return fetch_status;

  root_catalog_page->MarkDirty();
  std::memset(root_catalog_page->data(), 0, 1 << header_.page_shift);
  // TODO(pwnall): Bootstrap the root catalog here.
  page_pool_->UnpinAndWriteStorePage(root_catalog_page);

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

  // Rollback the init transaction to get the store's pages released.
  Status rollback_status = init_transaction_.Rollback();
  if (rollback_status != Status::kSuccess && result == Status::kSuccess)
    result = rollback_status;

  data_file_->Close();
  log_file_->Close();

  state_ = State::kClosed;
  page_pool_->pool()->StoreClosed(this);

  return result;
}

Status StoreImpl::ReadPage(Page* page) {
  DCHECK(page != nullptr);
  DCHECK(page->transaction() != nullptr);
  DCHECK_EQ(this, page->transaction()->store());
  DCHECK(!page->is_dirty());
  DCHECK(!page->IsUnpinned());

  size_t file_offset = page->page_id() << header_.page_shift;
  size_t page_size = 1 << header_.page_shift;
  return data_file_->Read(file_offset, page_size, page->data());
}

Status StoreImpl::WritePage(Page* page) {
  DCHECK(page != nullptr);
  DCHECK(page->transaction() != nullptr);
  DCHECK_EQ(this, page->transaction()->store());
  DCHECK(page->is_dirty());
  //DCHECK(!page->IsUnpinned());

  size_t file_offset = page->page_id() << header_.page_shift;
  size_t page_size = 1 << header_.page_shift;
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

#if DCHECK_IS_ON()
size_t StoreImpl::AssignedPageCount() noexcept {
  size_t count = init_transaction_.AssignedPageCount();
  for (TransactionImpl* transaction : transactions_)
    count += transaction->AssignedPageCount();
  return count;
}
#endif  // DCHECK_IS_ON()

}  // namespace berrydb
