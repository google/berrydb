// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "berrydb/pool.h"

#include "berrydb/platform.h"

#include "../pool_impl.h"
#include "../store_impl.h"

namespace berrydb {

// static
void Pool::operator delete(void* instance, size_t instance_size) {
  Deallocate(instance, instance_size);
}

std::unique_ptr<Pool> Pool::Create(const PoolOptions& options) {
  return PoolImpl::Create(options);
}

}  // namespace berrydb
