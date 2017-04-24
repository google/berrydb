// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "berrydb/options.h"

namespace berrydb {

PoolOptions::PoolOptions() : page_shift(15), page_pool_size(), vfs(nullptr) { }

StoreOptions::StoreOptions()
    : create_if_missing(true), error_if_exists(false) { }

}  // namespace berrydb
