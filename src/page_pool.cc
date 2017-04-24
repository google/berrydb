// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "./page_pool.h"

#include "berrydb/platform.h"

namespace berrydb {

PagePool::PagePool(size_t page_shift, size_t page_capacity)
    : page_shift_(page_shift), page_size_(1 << page_shift),
      page_capacity_(page_capacity), free_head_(this), lru_head_(this),
      log_head_(this) {
  DCHECK_EQ(page_size_ & (page_size_ - 1), 0);
}

}  // namespace berrydb
