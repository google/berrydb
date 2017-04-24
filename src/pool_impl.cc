// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "./pool_impl.h"

#include "berrydb/options.h"
#include "berrydb/platform.h"
#include "berrydb/vfs.h"
#include "./store_impl.h"

namespace berrydb {

static_assert(std::is_standard_layout<PoolImpl>::value,
    "PoolImpl must be a standard layout type so its public API can be "
    "exposed cheaply");

PoolImpl* PoolImpl::Create(const PoolOptions& options) {
  void* heap_block = Allocate(sizeof(PoolImpl));
  PoolImpl* pool = new (heap_block) PoolImpl(options);
  DCHECK_EQ(heap_block, static_cast<void*>(pool));
  return pool;
}

PoolImpl::PoolImpl(const PoolOptions& options)
    : api_(), page_pool_(options.page_shift, options.page_pool_size),
      vfs_((options.vfs == nullptr) ? DefaultVfs() : options.vfs) {
}

Status PoolImpl::OpenStore(
    const std::string& path, const StoreOptions& options,
    StoreImpl** result) {
  BlockAccessFile* data_file;
  Status status = vfs_->OpenForBlockAccess(
      path, page_pool_.page_shift(), options.create_if_missing,
      options.error_if_exists, &data_file);
  if (status != Status::kSuccess)
    return status;

  // TODO(pwnall): Lock the data file to prevent against accidental concurrent
  //               store opens.

  StoreImpl* store = StoreImpl::Create(data_file, &page_pool_, options);
  

  *result = store;
  return Status::kSuccess;
}

}  // namespace berrydb
