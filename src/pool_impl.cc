// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "./pool_impl.h"

#include "berrydb/options.h"
#include "berrydb/platform.h"
#include "berrydb/vfs.h"
#include "./store_impl.h"

namespace berrydb {

#ifndef _MSC_VER  // Visual Studio's std::standard_layout is buggy.
static_assert(std::is_standard_layout<PoolImpl>::value,
    "PoolImpl must be a standard layout type so its public API can be "
    "exposed cheaply");
#endif  // _MSC_VER

PoolImpl* PoolImpl::Create(const PoolOptions& options) {
  void* heap_block = Allocate(sizeof(PoolImpl));
  PoolImpl* pool = new (heap_block) PoolImpl(options);
  DCHECK_EQ(heap_block, static_cast<void*>(pool));
  return pool;
}

PoolImpl::PoolImpl(const PoolOptions& options)
    : api_(), page_pool_(this, options.page_shift, options.page_pool_size),
      vfs_((options.vfs == nullptr) ? DefaultVfs() : options.vfs) {
}

PoolImpl::~PoolImpl() { }

void PoolImpl::Release() {
  // Replace the entire store list so StoreClosed() doesn't invalidate our
  // iterator.
  StoreSet close_queue;
  close_queue.swap(stores_);
  for (StoreImpl* store : close_queue)
    store->Close();

  // The existence of pinned pages implies that some transactions are still
  // running. This should not be the case, as all the stores should have been
  // closed.
  DCHECK_EQ(page_pool_.pinned_pages(), 0U);

  // The difference between allocated pages and unused pages is pages in the LRU
  // queue. All the stores should have been closed, so the LRU should be empty.
  DCHECK_EQ(page_pool_.allocated_pages(), page_pool_.unused_pages());

  this->~PoolImpl();
  void* heap_block = static_cast<void*>(this);
  Deallocate(heap_block, sizeof(PoolImpl));
}

void PoolImpl::StoreCreated(StoreImpl* store) {
  DCHECK(store != nullptr);
  DCHECK(!store->IsClosed());
#if DCHECK_IS_ON()
  DCHECK_EQ(this, store->page_pool()->pool());
#endif  // DCHECK_IS_ON()

  // stores_.insert(store);
}

void PoolImpl::StoreClosed(StoreImpl* store) {
  DCHECK(store != nullptr);
  DCHECK(store->IsClosed());
#if DCHECK_IS_ON()
  DCHECK_EQ(this, store->page_pool()->pool());
#endif  // DCHECK_IS_ON()

  // TODO(pwnall): This probably needs the same open/closed/isClosing logic as
  //               StoreImpl::TransactionClosed().

  stores_.erase(store);
}

Status PoolImpl::OpenStore(
    const std::string& path, const StoreOptions& options,
    StoreImpl** result) {
  BlockAccessFile* data_file;
  size_t data_file_size;
  Status status = vfs_->OpenForBlockAccess(
      path, page_pool_.page_shift(), options.create_if_missing,
      options.error_if_exists, &data_file, &data_file_size);
  if (status != Status::kSuccess)
    return status;

  status = data_file->Lock();
  if (status != Status::kSuccess) {
    data_file->Close();
    return status;
  }

  std::string log_file_path = StoreImpl::LogFilePath(path);
  RandomAccessFile* log_file;
  size_t log_file_size;
  status = vfs_->OpenForRandomAccess(
      log_file_path, true /* create_if_missing */, false /* error_if_exists */,
      &log_file, &log_file_size);
  if (status != Status::kSuccess) {
    data_file->Close();
    return status;
  }

  StoreImpl* store = StoreImpl::Create(
      data_file, data_file_size, log_file, log_file_size, &page_pool_, options);
  stores_.insert(store);

  status = store->Initialize(options);
  if (status != Status::kSuccess) {
    store->Close();
    return status;
  }

  *result = store;
  return Status::kSuccess;
}

}  // namespace berrydb
