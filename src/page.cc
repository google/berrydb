// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "./page.h"

#include <type_traits>

#include "berrydb/platform.h"
#include "./page_pool.h"

namespace berrydb {

Page* Page::Create(PagePool* page_pool) {
  DCHECK(page_pool != nullptr);

  size_t block_size = sizeof(Page) + page_pool->page_size();
  void* page_block = Allocate(block_size);
  Page* page = new (page_block) Page(page_pool);
  DCHECK_EQ(reinterpret_cast<void*>(page), page_block);

  // Make sure that page data is 8-byte aligned.
  DCHECK_EQ(reinterpret_cast<uintptr_t>(page->data()) & 0x07, 0U);

  return page;
}

void Page::Release(PagePool *page_pool) {
#if DCHECK_IS_ON()
  DCHECK_EQ(page_pool_, page_pool);
#endif  // DCHECK_IS_ON()

  size_t block_size = sizeof(Page) + page_pool->page_size();
  void* heap_block = reinterpret_cast<void*>(this);
  Deallocate(heap_block, block_size);
}

Page::Page(PagePool* page_pool)
    : pin_count_(1)
#if DCHECK_IS_ON()
    , page_pool_(page_pool)
#endif  // DCHECK_IS_ON()
    {
  UNUSED(page_pool);

#if DCHECK_IS_ON()
  transaction_ = nullptr;
#endif
}

Page::~Page() {
  DCHECK(transaction_ == nullptr);
}

}  // namespace berrydb
