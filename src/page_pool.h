// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_PAGE_POOL_H_
#define BERRYDB_PAGE_POOL_H_

#include <cstddef>
#include <cstdint>
#include <functional>
#include <tuple>
#include <unordered_map>

#include "berrydb/status.h"
#include "./page.h"
#include "./store_impl.h"
#include "./transaction_impl.h"
#include "./util/checks.h"
#include "./util/linked_list.h"
#include "./util/platform_allocator.h"

namespace berrydb {

class PoolImpl;
class Store;
class StoreImpl;

/**
 * Manages buffers used as scratch pad and cache for a store's data pages.
 *
 * A store's data is distributed across equally-sized pages. The page size is a
 * power of two, so a page is guaranteed to contain an integer number of cache
 * lines and disk sectors.
 *
 * Access method algorithms that operate on store pages assume fast random
 * access to the data inside a page. This assumption is satisfied by copying the
 * pages whose contents needs to be accessed from disk into scratch buffers,
 * running the access method algorithms on the data in the scratch buffers, and
 * then writing any modified scratch buffer content back to disk.
 *
 * Reading page data from disk and writing it back to disk are both expensive
 * (time-consuming) operations, so it is desirable to amortize the operations'
 * costs over multiple access method algorithm invocations. This is accomplished
 * by holding onto page data into scratch buffers for as long as possible, which
 * effectively turns the buffers into a cache.
 *
 * Taking the above into consideration, a page pool is a specialized cache
 * memory whose entries are buffers that cache on-disk pages. A page pool can
 * cache pages from any number of different stores, as long as the stores have
 * the same page size. In fact, an application aiming for maximum efficiency
 * given a fixed memory budget will use a single page pool for all its open
 * stores.
 *
 * To speed up initialization, a page pool does not allocate memory for its
 * entire capacity when it is created. Instead, the entries are allocated as
 * they are needed. Conversely, when a store is closed, this frees up all the
 * entries caching the store's pages. To avoid heap churn, the free entries'
 * memory is not returned to the memory allocator. Instead, the page pool holds
 * onto these entries in an unused entry list, and reuses them.
 *
 * To make sure we don't evict the data in buffers that are currently used as
 * scratch space by some part of the system, every page pool user (component
 * that calls into PagePool) is responsible for maintaining a pin on the entries
 * that are used as scratch space. Page pool entries that have at least one pin
 * on them are pinned. Unpinned entries are tracked in an LRU cache, and can be
 * evicted at any time so, once a user releases its pin on an entry, it must not
 * touch that entry again.
 *
 * Page pool users are required to notify the pool when they modify a page pool
 * entry's data. Notifying is accomplished by marking the entry as dirty. The
 * page pool relies on the entries' dirty status to decide which entries have
 * their buffer contents written back to disk when they are evicted.
 *
 * The class representing a page pool entry is simply called Page, reflecting
 * the fact that most of the system only cares about the content of the entry's
 * buffer.
 *
 * Users call StorePage() to obtain a Page whose buffer holds the desired store
 * page. Users who intend to modify the Page's buffer call MarkDirty() _before_
 * making any changes to the buffer. When a user is done with a Page buffer, it
 * calls UnpinStorePage(), so the page pool entry can become eligible for
 * eviction again.
 */
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

  /** Desired behavior when unpinning a page makes it eligible for eviction. */
  enum PageUnpinMode : bool {
    /* Cache the page's content in the pool using normal caching rules.
     *
     * This is the default value, and most callers should stick to it. */
    kCachePage = false,

    /* Discard the page from the pool as soon as space is needed.
     *
     * This should be used when iterating over large data sets, to avoid
     * poisoning the cache with bad data. */
    kDiscardPage = true,
  };

  /** Sets up a page pool. Page memory may be allocated on-demand. */
  PagePool(PoolImpl* pool, size_t page_shift, size_t page_capacity);

  /** Deallocates the memory used by the pool's pages. */
  ~PagePool();

  PagePool(const PagePool&) = delete;
  PagePool(Page&&) = delete;
  PagePool& operator=(const PagePool&) = delete;
  PagePool& operator=(PagePool&&) = delete;

  /** Fetches a page from a store and pins it.
   *
   * The caller owns a pin of the page, and must remove the pin by calling
   * UnpinStorePage() after using the page.
   *
   * If the desired fetch mode is kIgnorePageData, the returned page may or may
   * not be marked dirty. Thus, callers that use kIgnorePageData must also call
   * MarkDirty() on the result page.
   *
   * @param  store      the store to fetch a page from
   * @param  page_id    the page that will be fetched from the store
   * @param  fetch_mode desired fetching behavior
   * @return status     may return kPoolFull if the page pool is (almost) full
   *                    and cannot find a free page, or kIoError if reading the
   *                    store page failed
   * @return page       if the call succeeds, points to the page pool entry
   *                    holding the page */
  std::tuple<Status, Page*> StorePage(StoreImpl* store, size_t page_id,
                                      PageFetchMode fetch_mode);

  /** Releases a Page previously obtained by StorePage().
   *
   * The method removes the caller's pin from this pool page entry. The page
   * entry might still be pinned by other entities. For example, a cursor from
   * the same transaction would hold a pin to its current page. Furthermore,
   * readonly transactions can happen concurrently, so a page entry might be
   * used by cursors from multiple readonly transactions.
   *
   * If the last pin is removed, the page entry will eventually cache another
   * store page. However, for a short while, the entry will end up in the LRU
   * queue, and remain associated with the store. Sadly, this means that the
   * calling code may be able to access the page entry's data without errors.
   * Nevertheless, the caller must not use the page entry anymore after
   * releasing its pin.
   *
   * @param  page a page pool entry that was previously obtained from this pool
   *              using StorePage()
   * @param  mode the desired behavior when unpinning makes this page pool
   *              entry's cached data eligible for eviction */
  inline void UnpinStorePage(Page* page,
                             PageUnpinMode mode = kCachePage) noexcept {
    BERRYDB_ASSUME(page != nullptr);
    BERRYDB_ASSUME(page->transaction() != nullptr);
    BERRYDB_ASSUME(page->transaction()->store() != nullptr);
#if BERRYDB_CHECK_IS_ON()
    BERRYDB_CHECK_EQ(page->page_pool(), this);
#endif  // BERRYDB_CHECK_IS_ON()

    page->RemovePin();
    if (page->IsUnpinned()) {
      // NOTE: This looks like a lot of code for an inlined function. However,
      //       all call sites will specify a constant mode, so one branch will
      //       always be optimized out.
      if (mode == kCachePage)
        lru_list_.push_back(page);
      else
        lru_list_.push_front(page);
    }
  }

  /** Releases and writes back a dirty Page previously obtained by StorePage().
   *
   * This is similar to UnpinStorePage(), but the caller is supplying an extra
   * hint that the page is dirty, and must be written back to the store's data
   * file now. This is rather rare, as in general it is advantageous to batch
   * writes, which implies keeping dirty pages in the LRU cache for as long as
   * possible.
   *
   * @param  page a dirty page that was previously obtained from this pool using
   *             StorePage() */
  void UnpinAndWriteStorePage(Page* page);

  /** The base-2 log of the pool's page size. */
  inline constexpr size_t page_shift() const noexcept { return page_shift_; }

  /** Size of a page. Guaranteed to be a power of two. */
  inline constexpr size_t page_size() const noexcept { return page_size_; }

  /** Maximum number of pages cached by this page pool. */
  inline constexpr size_t page_capacity() const noexcept {
    return page_capacity_;
  }

  /** Total number of pages allocated for this pool. */
  inline constexpr size_t allocated_pages() const noexcept {
    return page_count_;
  }

  /** Number of pages that were allocated and are now unused.
   *
   * Pool pages can become unused when a store is closed or experiences I/O
   * errors. These pages are added to a free list, so future demand can be met
   * without invoking the platform allocator.
   */
  inline constexpr size_t unused_pages() const noexcept {
    return free_list_.size();
  }

  /** Number of pages that are pinned by running transactions.
   *
   * Only unpinned pages can be evicted and reused to meed demands for new
   * pages. If all pages in the pool become pinned, transactions that need more
   * page pool entries will be rolled back. */
  inline constexpr size_t pinned_pages() const noexcept {
    return page_count_ - free_list_.size() - lru_list_.size();
  }

  /** The resource pool that this page pool belongs to. */
  inline constexpr PoolImpl* pool() const noexcept { return pool_; }

  /**
   * Allocates a page and pins it.
   *
   * This method is intended for allocating pages that will end up holding log
   * data, and for internal use. Store pages should be handled using StorePage()
   * and UnpinStorePage().
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
   * The store page must not already be cached in this page pool. The caller
   * must have a pin on the page pool entry. This method is intended for
   * internal and testing use.
   *
   * @param  page       a page pool entry that is not associated with a store
   * @param  store      the store to fetch a page from
   * @param  page_id    the page that will be fetched from the store
   * @param  fetch_mode desired fetching behavior
   * @return            most likley kSuccess or kIoError
   */
  Status AssignPageToStore(Page* page,
                           StoreImpl* store,
                           size_t page_id,
                           PageFetchMode fetch_mode);

  /** Frees up a page pool entry that is currently caching a store page.
   *
   * Most callers should use UnpinStorePage() instead. This method guarantees
   * that the pool entry will not be caching the store page.
   *
   * @param page the page pool entry to be freed
   */
  void UnassignPageFromStore(Page* page);

  /** Adds a pin to a pool entry that is currently caching a store page.
   *
   * This is intended for internal use and for testing.
   *
   * @param page the page pool entry that will receive an extra pin
   */
  void PinStorePage(Page* page);

  /** Acquires pins on all the pages in a transaction's page list.
   *
   * After this method returns, the list of pages assigned to the transaction is
   * guaranteed to be stable, assuming that the transaction refuses to fetch new
   * pages.
   *
   * @param page_list the list of pages to acquire pins on
   */
  void PinTransactionPages(
      LinkedList<Page, Page::TransactionLinkedListBridge>* page_list);

 private:
  /** Entries that belong to this page pool that are assigned to stores. */
  using PageMapKey = std::pair<StoreImpl*, size_t>;
  std::unordered_map<PageMapKey,
                     Page*,
                     PointerSizeHasher<StoreImpl>,
                     std::equal_to<PageMapKey>,
                     PlatformAllocator<std::pair<const PageMapKey, Page*>>>
      page_map_;

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
