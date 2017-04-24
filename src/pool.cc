// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "berrydb/pool.h"

#include "./pool_impl.h"

namespace berrydb {

Pool* Pool::Create(const PoolOptions& options) {
  return PoolImpl::Create(options)->ToPool();
}

}  // namespace berrydb
