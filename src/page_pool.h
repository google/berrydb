// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_PAGE_POOL_H_
#define BERRYDB_PAGE_POOL_H_

#include <cstddef>
#include <cstdint>
#include <functional>
#include <unordered_map>

#include "berrydb/platform.h"
#include "berrydb/status.h"
#include "./page.h"
#include "./util/platform_allocator.h"

namespace berrydb {

class Store;

/** In-memory page cache. */
class PagePool {

 public:
  /** Sets up a page pool. Page memory may be allocated on-demand. */
  PagePool(size_t page_shift, size_t page_capacity);

  /** Size of a page. Guaranteed to be a power of two. */
  inline size_t page_size() const noexcept { return page_size_; }

  /** Maximum number of pages cached by this page pool. */
  inline size_t page_capacity() const noexcept { return page_capacity_; }

  /** Fetches a page from a store and pins it.
   *
   * Returns nullptr if the pool cannot find a page. This happens if the pool is
   * (almost) full of pinned pages.
   *
   * The caller must unpin the page after using it. */
  Page* Pin(Store* store, size_t page_id);

  /** Releases a Page previously obtained by Pin().
   *
   * The page might stll be in the cache.
   *
   * @param page [description]
   */
  void Unpin(Page* page);

  /**
   * Allocates a page and pins it.
   *
   * This method is intended for allocating pages that will end up holding log
   * data, and for internal use. Store pages should be handled using Pin() and
   * Unpin().
   *
   * Returns nullptr if the pool cannot find a page. This happens if the pool is
   * (almost) full of pinned pages.
   * @return [description]
   */
  Page* Alloc();

 private:
  typedef std::pair<Store*, size_t> PageMapKey;
  std::unordered_map<PageMapKey, Page*, PointerSizeHasher<Store>,
      std::equal_to<PageMapKey>,
      PlatformAllocator<std::pair<const PageMapKey, Page*>>> page_map_;

  /** The page size is (1 << page_shift_). */
  size_t page_shift_;

  /** Size of a page. Guaranteed to be a power of two. */
  size_t page_size_;

  /** Maximum number of pages that the pool will hold. */
  size_t page_capacity_;

  /** Number of pages currently held by the pool. */
  size_t page_count_;

  /** Sentinel for the list of pages that haven't been returned to the OS. */
  Page free_head_;

  /** Pages that can be evicted. */
  Page lru_head_;

  /** Log pages waiting to be written to disk. */
  Page log_head_;
};

}  // namespace berrydb

#endif  // BERRYDB_PAGE_POOL_H_
