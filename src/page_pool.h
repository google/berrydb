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
class StoreImpl;

/** In-memory page cache. */
class PagePool {
 public:
  /** Desired outcome if a requested store page is not already in the pool. */
  enum PageFetchMode : bool {
    /** Read the missing page from the store's data file.
     *
     * Intended for callers who use the page data. This is the correct outcome
     * almost all the time. */
    kFetchPageData = true,

    /** Skip reading the missing page from the store.
     *
     * Intended for callers who intend to overwrite the page without reading it.
     */
    kIgnorePageData = true,
  };

  /** Sets up a page pool. Page memory may be allocated on-demand. */
  PagePool(size_t page_shift, size_t page_capacity);

  /** The base-2 log of the pool's page size. */
  inline size_t page_shift() const noexcept { return page_shift_; }

  /** Size of a page. Guaranteed to be a power of two. */
  inline size_t page_size() const noexcept { return page_size_; }

  /** Maximum number of pages cached by this page pool. */
  inline size_t page_capacity() const noexcept { return page_capacity_; }

  /** Fetches a page from a store and pins it.
   *
   * The caller owns a pin of the page, and must remove the pin by calling
   * UnpinStorePage() after using the page.
   *
   * @param  store      the store to fetch a page from
   * @param  page_id    the page that will be fetched from the store
   * @param  fetch_mode desired behavior
   * @param  result     if the call succeeds, will receive a pointer to the page
   *                    pool entry holding the page
   * @return            may return kPoolFull if the page pool is (almost) full
   *                    and cannot find a free page, or kIoError if reading the
   *                    store page failed */
  Status StorePage(
      StoreImpl* store, size_t page_id, PageFetchMode fetch_mode,
      Page** result);

  /** Releases a Page previously obtained by StorePage().
   *
   * The page might still be in the cache.
   *
   * @param page a page that was previously obtained from this pool using
   *             StorePage()
   */
  void UnpinStorePage(Page* page);

  /**
   * Allocates a page and pins it.
   *
   * This method is intended for allocating pages that will end up holding log
   * data, and for internal use. Store pages should be handled using Pin() and
   * Unpin().
   *
   * The caller is responsible for reducing the page's pin count.
   *
   * @return a pinned page, or nullptr if the pool cannot find a page - this
   *         means the pool is (almost) full of pinned pages.
   */
  Page* AllocPage();

  /** Reads a pool entry's page data from its associated store.
   *
   * @param  page       a page pool entry that is associated with a store
   * @param  fetch_mode if kIgnorePageData, marks the page as dirty instead of
   *                    reading it from the store
   * @return            most likely kSuccess or kIoError
   */
  Status FetchStorePage(Page* page, PageFetchMode fetch_mode);

 private:
  typedef std::pair<StoreImpl*, size_t> PageMapKey;
  std::unordered_map<PageMapKey, Page*, PointerSizeHasher<StoreImpl>,
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

  /** Sentinel for the list of pages that haven't been returned to the OS.
   *
   * This is only populated when a Store is closed and its pages are flushed
   * from the pool.
   */
  Page free_sentinel_;

  /** Pages that can be evicted, ordered by the relative time of last use.
   *
   * The first page in the list is the most recently used (MRU) page. Therefore,
   * the LRU cache replacement policy should be implemented by removing the last
   * page in this list.
   */
  Page mru_sentinel_;

  /** Log pages waiting to be written to disk. */
  Page log_sentinel_;
};

}  // namespace berrydb

#endif  // BERRYDB_PAGE_POOL_H_
