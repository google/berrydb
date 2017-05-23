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
#include "./util/linked_list.h"
#include "./util/platform_allocator.h"

namespace berrydb {

class PoolImpl;
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
    kIgnorePageData = false,
  };

  /** Sets up a page pool. Page memory may be allocated on-demand. */
  PagePool(PoolImpl* pool, size_t page_shift, size_t page_capacity);

  /** Deallocates the memory used by the pool's pages. */
  ~PagePool();

  /** Fetches a page from a store and pins it.
   *
   * The caller owns a pin of the page, and must remove the pin by calling
   * UnpinStorePage() after using the page.
   *
   * @param  store      the store to fetch a page from
   * @param  page_id    the page that will be fetched from the store
   * @param  fetch_mode desired fetching behavior
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

  /** The base-2 log of the pool's page size. */
  inline size_t page_shift() const noexcept { return page_shift_; }

  /** Size of a page. Guaranteed to be a power of two. */
  inline size_t page_size() const noexcept { return page_size_; }

  /** Maximum number of pages cached by this page pool. */
  inline size_t page_capacity() const noexcept { return page_capacity_; }

  /** Total number of pages allocated for this pool. */
  inline size_t allocated_pages() const noexcept { return page_count_; }

  /** Number of pages that were allocated and are now unused.
   *
   * Pool pages can become unused when a store is closed or experiences I/O
   * errors. These pages are added to a free list, so future demand can be met
   * without invoking the platform allocator.
   */
  inline size_t unused_pages() const noexcept { return free_list_.size(); }

  /** Number of pages that are pinned by running transactions.
   *
   * Only unpinned pages can be evicted and reused to meed demands for new
   * pages. If all pages in the pool become pinned, transactions that need more
   * page pool entries will be aborted. */
  inline size_t pinned_pages() const noexcept {
    return page_count_ - free_list_.size() - lru_list_.size();
  }

  /** The resource pool that this page pool belongs to. */
  inline PoolImpl* pool() const noexcept { return pool_; }

  /**
   * Allocates a page and pins it.
   *
   * This method is intended for allocating pages that will end up holding log
   * data, and for internal use. Store pages should be handled using Pin() and
   * Unpin().
   *
   * The caller is responsible for reducing the page's pin count.
   *
   * @return a pinned page, or nullptr if the pool is at capacity
   */
  Page* AllocPage();

  /** Releases a Page previously obtained by Alloc().
   *
   * This method is intended for internal and testing use.
   *
   * @param page a page that was not assigned to cache a store page
   */
  void UnpinUnassignedPage(Page* page);

  /** Reads a pool entry's page data from its associated store.
   *
   * This method is intended for internal and testing use.
   *
   * @param  page       a page pool entry that is associated with a store
   * @param  fetch_mode if kIgnorePageData, marks the page as dirty instead of
   *                    reading it from the store
   * @return            most likely kSuccess or kIoError
   */
  Status FetchStorePage(Page* page, PageFetchMode fetch_mode);

  /** Assigns a page pool entry to cache a store page.
   *
   * The store page must not already be cached in this page pool. This method is
   * intended for internal and testing use.
   *
   * @param  page       a page pool entry that is not associated with a store
   * @param  store      the store to fetch a page from
   * @param  page_id    the page that will be fetched from the store
   * @param  fetch_mode desired fetching behavior
   * @return            [description]
   */
  Status AssignPageToStore(
      Page* page, StoreImpl* store, size_t page_id, PageFetchMode fetch_mode);

  /** Frees up a page pool entry that is currently caching a store page.
   *
   * @param page the page pool entry to be freed
   */
  void UnassignPageFromStore(Page* page);

 private:
  /** Entries that belong to this page pool that are assigned to stores. */
  using PageMapKey = std::pair<StoreImpl*, size_t>;
  std::unordered_map<PageMapKey, Page*, PointerSizeHasher<StoreImpl>,
      std::equal_to<PageMapKey>,
      PlatformAllocator<std::pair<const PageMapKey, Page*>>> page_map_;

  size_t page_shift_;
  size_t page_size_;
  size_t page_capacity_;
  PoolImpl* const pool_;

  /** Number of pages currently held by the pool. */
  size_t page_count_ = 0;

  /** The list of pages that haven't been returned to the OS.
   *
   * This is only populated when a Store is closed and its pages are flushed
   * from the pool.
   */
  LinkedList<Page> free_list_;

  /** Pages that can be evicted, ordered by the relative time of last use.
   *
   * The first page in the list is the least recently used (LRU) page. The LRU
   * cache replacement policy should be implemented by removing the first page
   * in this list (pop_front), and pages should be added at the end of the list
   * (push_back).
   */
  LinkedList<Page> lru_list_;

  /** Log pages waiting to be written to disk. */
  LinkedList<Page> log_list_;
};

}  // namespace berrydb

#endif  // BERRYDB_PAGE_POOL_H_
