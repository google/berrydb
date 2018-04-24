// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "berrydb/pool.h"

#include "../pool_impl.h"
#include "../store_impl.h"

namespace berrydb {

Pool* Pool::Create(const PoolOptions& options) {
  return PoolImpl::Create(options)->ToApi();
}

void Pool::Release() {
  return PoolImpl::FromApi(this)->Release();
}

std::tuple<Status, Store*> Pool::OpenStore(const std::string& path,
                                           const StoreOptions& options) {
  Status status;
  StoreImpl* store;
  std::tie(status, store) = PoolImpl::FromApi(this)->OpenStore(path, options);
  return {status, store->ToApi()};
}

size_t Pool::page_size() const {
  return PoolImpl::FromApi(this)->page_size();
}

size_t Pool::page_pool_size() const {
  return PoolImpl::FromApi(this)->page_pool_size();
}

}  // namespace berrydb
