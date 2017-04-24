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
  Page* page = new (page_block) Page(page_pool, nullptr /* is_not_sentinel */);
  DCHECK_EQ(reinterpret_cast<void*>(page), page_block);

  return page;
}

Page::Page(PagePool* page_pool, std::nullptr_t is_not_sentinel)
    : pin_count_(1), is_dirty_(false)
#if DCHECK_IS_ON()
    , page_pool_(page_pool), is_sentinel_(false)
#endif  // DCHECK_IS_ON()
    {
#if DCHECK_IS_ON()
  next_ = nullptr;
  prev_ = nullptr;
#endif
}

Page::Page(PagePool* page_pool)
    : next_(this), prev_(this),  // List sentinel initialization
      store_(nullptr), page_id_(0), pin_count_(kSentinelPinCount),
      is_dirty_(false)
#if DCHECK_IS_ON()
    , page_pool_(page_pool), is_sentinel_(true)
#endif  // DCHECK_IS_ON()
      {
  // The store_ and pin_count_ values below are chosen to trigger DCHECKs if the
  // sentinel is accidentally mistaken for a real page. We could skip their
  // initialization when DCHECKs are disabled, but sentinel page stubs are
  // created very rarely, so initializing the members all the time is an
  // insignificant performance hit.
}

}  // namespace berrydb
