// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "./store_impl.h"

#if BERRYDB_CHECK_IS_ON()
#include <ostream>  // Needed by BERRYDB_ASSUME_EQ(State, State).
#endif  // BERRYDB_CHECK_IS_ON()
#include <tuple>

#include "berrydb/options.h"
#include "berrydb/platform.h"
#include "berrydb/vfs.h"
#include "./free_page_list.h"
#include "./pinned_page.h"
#include "./pool_impl.h"
#include "./transaction_impl.h"
#include "./util/checks.h"
#include "./util/span_util.h"

namespace berrydb {

#if BERRYDB_CHECK_IS_ON()

namespace {

const char* StoreImplStateToCString(StoreImpl::State state) noexcept {
  switch (state) {
  case StoreImpl::State::kClosed:
    return "Closed";
  case StoreImpl::State::kClosing:
    return "Closing";
  case StoreImpl::State::kOpen:
    return "Open";
  }

  BERRYDB_UNREACHABLE();
}

}  // namespace

// Needed by BERRYDB_ASSUME_EQ(State, State)
std::ostream& operator<<(std::ostream& stream, StoreImpl::State state) {
  return stream << "[State: " << StoreImplStateToCString(state) << "]";
}

#endif  // BERRYDB_CHECK_IS_ON()

static_assert(std::is_standard_layout<StoreImpl>::value,
    "StoreImpl must be a standard layout type so its public API can be "
    "exposed cheaply");

StoreImpl* StoreImpl::Create(
    BlockAccessFile* data_file, size_t data_file_size,
    RandomAccessFile* log_file, size_t log_file_size, PagePool* page_pool,
    const StoreOptions& options) {
  void* const heap_block = Allocate(sizeof(StoreImpl));
  StoreImpl* const store = new (heap_block) StoreImpl(
      data_file, data_file_size, log_file, log_file_size, page_pool, options);
  BERRYDB_ASSUME_EQ(heap_block, static_cast<void*>(store));

  page_pool->pool()->StoreCreated(store);
  return store;
}

void StoreImpl::Release() {
  this->~StoreImpl();
  void* const heap_block = static_cast<void*>(this);
  Deallocate(heap_block, sizeof(StoreImpl));
}

StoreImpl::StoreImpl(
    BlockAccessFile* data_file, size_t data_file_size,
    RandomAccessFile* log_file,
    // This will be used when we implement log recovery.
    MAYBE_UNUSED size_t log_file_size,
    PagePool* page_pool,
    // This will be used when we implement creating/loading the metadata page.
    MAYBE_UNUSED const StoreOptions& options)
    : data_file_(data_file), log_file_(log_file), page_pool_(page_pool),
      init_transaction_(this, true), header_(
          page_pool->page_shift(), data_file_size >> page_pool->page_shift()) {
  BERRYDB_ASSUME(data_file != nullptr);
  BERRYDB_ASSUME(log_file != nullptr);
  BERRYDB_ASSUME(page_pool != nullptr);
}

StoreImpl::~StoreImpl() {
  if (state_ == State::kOpen)
    Close();

  BERRYDB_ASSUME_EQ(state_, State::kClosed);
}

Status StoreImpl::Initialize(const StoreOptions &options) {
  // TODO(pwnall): Check the log and attempt recovery.

  if (options.create_if_missing && header_.page_count < 3) {
    const Status status = Bootstrap();
    if (UNLIKELY(status != Status::kSuccess))
      return status;
  }

  return Status::kSuccess;
}

Status StoreImpl::Bootstrap() {
  BERRYDB_ASSUME_EQ(page_pool_->page_shift(), header_.page_shift);

  TransactionImpl* const transaction = CreateTransaction();

  Status fetch_status;
  {
    Page* raw_header_page;
    std::tie(fetch_status, raw_header_page) = page_pool_->StorePage(
        this, 0, PagePool::kIgnorePageData);
    if (UNLIKELY(fetch_status != Status::kSuccess)) {
      BERRYDB_ASSUME(raw_header_page == nullptr);
      transaction->Release();  // Rolls back the transaction.
      return fetch_status;
    }
    const PinnedPage header_page(raw_header_page, page_pool_);

    transaction->WillModifyPage(header_page.get());
    const span<uint8_t> header_page_data = header_page.mutable_data();
    FillSpan(header_page_data, 0);
    header_.free_list_head_page = FreePageList::kInvalidPageId;
    header_.page_count = 2;
    // header.page_shift is already set correctly by the constructor.
    header_.Serialize(header_page_data);
  }

  {
    Page* raw_root_catalog_page;
    std::tie(fetch_status, raw_root_catalog_page) = page_pool_->StorePage(
        this, 1, PagePool::kIgnorePageData);
    if (UNLIKELY(fetch_status != Status::kSuccess)) {
      BERRYDB_ASSUME(raw_root_catalog_page == nullptr);
      transaction->Release();  // Rolls back the transaction.
      return fetch_status;
    }
    const PinnedPage root_catalog_page(raw_root_catalog_page, page_pool_);

    transaction->WillModifyPage(root_catalog_page.get());
    const span<uint8_t> root_catalog_page_data =
        root_catalog_page.mutable_data();
    FillSpan(root_catalog_page_data, 0);
    // TODO(pwnall): Bootstrap the root catalog here.
  }

  const Status commit_status = transaction->Commit();
  if (commit_status != Status::kSuccess)
    return commit_status;

  transaction->Release();
  return Status::kSuccess;
}

TransactionImpl* StoreImpl::CreateTransaction() {
  TransactionImpl* const transaction = TransactionImpl::Create(this);
  transactions_.push_back(transaction);
  return transaction;
}

Status StoreImpl::Close() {
  if (UNLIKELY(state_ != State::kOpen)) {
    if (state_ == State::kClosed)
      return Status::kAlreadyClosed;
    else
      return Status::kSuccess;
  }

  // We cannot transition directly into the closed state because we want to
  // roll back the live transactions cleanly, assuming no I/O errors.
  state_ = State::kClosing;

  // Replace the entire transaction list so TransactionClosed() doesn't
  // invalidate our iterator.
  LinkedList<TransactionImpl> rollback_queue = std::move(transactions_);

  Status result = Status::kSuccess;
  for (TransactionImpl* transaction : rollback_queue) {
    Status rollback_status = transaction->Rollback();

    // Report the first non-success status encountered while rolling back the
    // running transactions. If an I/O error occurs, the first transaction will
    // rollback with kIoError, but the following transactions will rollback with
    // kAlreadyClosed. It's nice to return kIoError in this case.
    if (UNLIKELY(rollback_status != Status::kSuccess) &&
        result == Status::kSuccess) {
      result = rollback_status;
    }
  }

  // Rollback the init transaction to get the store's pages released.
  Status rollback_status = init_transaction_.Rollback();
  if (UNLIKELY(rollback_status != Status::kSuccess)
      && result == Status::kSuccess)
    result = rollback_status;

  data_file_->Close();
  log_file_->Close();

  state_ = State::kClosed;
  page_pool_->pool()->StoreClosed(this);

  return result;
}

Status StoreImpl::ReadPage(Page* page) {
  BERRYDB_ASSUME(page != nullptr);
  BERRYDB_ASSUME(page->transaction() != nullptr);
  BERRYDB_ASSUME_EQ(this, page->transaction()->store());
  BERRYDB_ASSUME(!page->is_dirty());
  BERRYDB_ASSUME(!page->IsUnpinned());

  const size_t file_offset = page->page_id() << header_.page_shift;
  const size_t page_size = static_cast<size_t>(1) << header_.page_shift;
  return data_file_->Read(file_offset, page->mutable_data(page_size));
}

Status StoreImpl::WritePage(Page* page) {
  BERRYDB_ASSUME(page != nullptr);
  BERRYDB_ASSUME(page->transaction() != nullptr);
  BERRYDB_ASSUME_EQ(this, page->transaction()->store());
  BERRYDB_ASSUME(page->is_dirty());
  // BERRYDB_ASSUME(!page->IsUnpinned());

  const size_t file_offset = page->page_id() << header_.page_shift;
  const size_t page_size = static_cast<size_t>(1) << header_.page_shift;
  return data_file_->Write(page->data(page_size), file_offset);
}

void StoreImpl::TransactionClosed(TransactionImpl* transaction) {
  BERRYDB_ASSUME(transaction != nullptr);
  BERRYDB_ASSUME(transaction->IsClosed());
#if BERRYDB_CHECK_IS_ON()
  BERRYDB_CHECK_EQ(this, transaction->store());
#endif  // BERRYDB_CHECK_IS_ON()

  BERRYDB_ASSUME_NE(state_, State::kClosed);
  if (state_ != State::kOpen)
    return;

  transactions_.erase(transaction);
}

std::string StoreImpl::LogFilePath(const std::string& store_path) {
  std::string log_path = store_path;
  log_path.append(".log", 4);
  return log_path;
}

#if BERRYDB_CHECK_IS_ON()
size_t StoreImpl::AssignedPageCount() noexcept {
  size_t count = init_transaction_.AssignedPageCount();
  for (TransactionImpl* transaction : transactions_)
    count += transaction->AssignedPageCount();
  return count;
}
#endif  // BERRYDB_CHECK_IS_ON()

}  // namespace berrydb
