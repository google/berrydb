// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "./pool_impl.h"

#include "berrydb/platform.h"

namespace berrydb {

static_assert(std::is_standard_layout<PoolImpl>::value,
    "PoolImpl must be a standard layout type so its public API can be "
    "exposed cheaply");

PoolImpl* PoolImpl::Create(const PoolOptions& options) {
  void* heap_block = Allocate(sizeof(PoolImpl));
  PoolImpl* pool_impl = new (heap_block) PoolImpl(options);
  DCHECK_EQ(heap_block, static_cast<void*>(pool_impl));
  return pool_impl;
}

PoolImpl::PoolImpl(const PoolOptions& options)
    : api_(), page_pool_(options.page_shift, options.page_pool_size) {
}

}  // namespace berrydb
