// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "./pool_impl.h"

#include "berrydb/options.h"
#include "berrydb/vfs.h"
#include "./store_impl.h"
#include "./util/checks.h"

namespace berrydb {

std::unique_ptr<PoolImpl> PoolImpl::Create(const PoolOptions& options) {
  return std::make_unique<PoolImpl>(options, PassKey());
}

PoolImpl::PoolImpl(const PoolOptions& options, PassKey)
    : Pool(PassKey()),
      page_pool_(this, options.page_shift, options.page_pool_size),
      vfs_((options.vfs == nullptr) ? DefaultVfs() : options.vfs) {
}

PoolImpl::~PoolImpl() {
  // Replace the entire store list so StoreClosed() doesn't invalidate our
  // iterator.
  StoreSet close_queue = std::move(stores_);
  for (StoreImpl* store : close_queue)
    store->Close();

  // The existence of pinned pages implies that some transactions are still
  // running. This should not be the case, as all the stores should have been
  // closed.
  BERRYDB_ASSUME_EQ(page_pool_.pinned_pages(), 0U);

  // The difference between allocated pages and unused pages is pages in the LRU
  // queue. All the stores should have been closed, so the LRU should be empty.
  BERRYDB_ASSUME_EQ(page_pool_.allocated_pages(), page_pool_.unused_pages());
}

// static
void* PoolImpl::operator new(size_t instance_size) {
  return Allocate(instance_size);
}

void PoolImpl::StoreCreated(StoreImpl* store) {
  BERRYDB_ASSUME(store != nullptr);
  BERRYDB_ASSUME(!store->IsClosed());
#if BERRYDB_CHECK_IS_ON()
  BERRYDB_CHECK_EQ(this, store->page_pool()->pool());
#endif  // BERRYDB_CHECK_IS_ON()

  stores_.insert(store);
}

void PoolImpl::StoreClosed(StoreImpl* store) {
  BERRYDB_ASSUME(store != nullptr);
  BERRYDB_ASSUME(store->IsClosed());
#if BERRYDB_CHECK_IS_ON()
  BERRYDB_CHECK_EQ(this, store->page_pool()->pool());
#endif  // BERRYDB_CHECK_IS_ON()

  // TODO(pwnall): This probably needs the same open/closed/isClosing logic as
  //               StoreImpl::TransactionClosed().

  stores_.erase(store);
}

std::tuple<Status, Store*> PoolImpl::OpenStore(
    const std::string& path, const StoreOptions& options) {
  Status status;
  BlockAccessFile* data_file;
  size_t data_file_size;
  std::tie(status, data_file, data_file_size) = vfs_->OpenForBlockAccess(
      path, page_pool_.page_shift(), options.create_if_missing,
      options.error_if_exists);
  if (UNLIKELY(status != Status::kSuccess))
    return {status, nullptr};

  status = data_file->Lock();
  if (UNLIKELY(status != Status::kSuccess)) {
    data_file->Close();
    return {status, nullptr};
  }

  const std::string log_file_path = StoreImpl::LogFilePath(path);
  RandomAccessFile* log_file;
  size_t log_file_size;
  std::tie(status, log_file, log_file_size) = vfs_->OpenForRandomAccess(
      log_file_path, true /* create_if_missing */, false /* error_if_exists */);
  if (UNLIKELY(status != Status::kSuccess)) {
    data_file->Close();
    return {status, nullptr};
  }

  StoreImpl* const store = StoreImpl::Create(
      data_file, data_file_size, log_file, log_file_size, &page_pool_, options);
  stores_.insert(store);

  status = store->Initialize(options);
  if (UNLIKELY(status != Status::kSuccess)) {
    store->Close();
    return {status, nullptr};
  }

  return {Status::kSuccess, store->ToApi()};
}

size_t PoolImpl::PageSize() const noexcept {
  return page_pool_.page_size();
}
size_t PoolImpl::PagePoolSize() const noexcept {
  return page_pool_.page_capacity();
}

}  // namespace berrydb
