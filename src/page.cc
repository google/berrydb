// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "./page.h"

#include <type_traits>

#include "berrydb/platform.h"
#include "./page_pool.h"

namespace berrydb {

Page* Page::Create(PagePool* page_pool) {
  size_t block_size = sizeof(Page) + page_pool->page_size();
  void* page_block = Allocate(block_size);
  Page* page = new (page_block) Page();
  DCHECK_EQ(reinterpret_cast<void*>(page), page_block);

#if DCHECK_IS_ON()
  page->pool_ = page_pool;
#endif  // DCHECK_IS_ON()

  return page;
}

Page::Page() : pin_count_(0) {
#if DCHECK_IS_ON()
  next_ = nullptr;
  prev_ = nullptr;
#endif  // DCHECK_IS_ON()
}

Page::Page(PagePool* page_pool)
    : next_(this), prev_(this),  // List sentinel initialization
    // The values below are chosen to trigger DCHECKs if the sentinel is
    // accidentally mistaken for a real page. The page ID is initialized to a
    // recognizable pattern.
    store_(nullptr), page_id_(0x80000000), pin_count_(1) {
#if DCHECK_IS_ON()
  pool_ = page_pool;
#endif  // DCHECK_IS_ON()
}

}  // namespace berrydb
