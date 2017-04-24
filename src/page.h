// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_PAGE_H_
#define BERRYDB_PAGE_H_

#include <cstddef>
#include <cstdint>

#include "berrydb/platform.h"

#include "./store_impl.h"  // Used in DCHECKs.

namespace berrydb {

class PagePool;
class StoreImpl;

/** Control information about an in-memory page.
 *
 * A page belongs to the same PagePool for its entire lifetime. The page does
 * not hold a reference to the pool (in release mode) to save space.
 *
 * Each page has a pin count. This is very similar to a reference count. While a
 * page is pinned, its contents cannot be replaced. Therefore, pinned pages
 * should not be in the pool's LRU queue.
 *
 * Most pages will be stored in a doubly linked list used to implement the LRU
 * eviction policy. For this reason, the next and prev pointers used to
 * implement the list are stored directly in the Page control structure.
 *
 * The control structures for real pages are followed immediately by the bytes
 * used to cache page data. The stubs used to implement the linked list
 * sentinels are never used to store real pages.
 */
class Page {
  enum class Status;

 public:
  /** Allocates a page that will belong to the given pool.
   *
   * The returned page has one pin on it, which is owned by the caller. */
  static Page* Create(PagePool* page_pool);

  /** Creates a stub page that can be used as a list sentinel. */
  Page(PagePool* page_pool);

  /** The store whose data is cached by this page. */
  inline StoreImpl* store() const noexcept { return store_; }

  /** The page ID of the store page whose data is cached by this pool page.
   *
   * This is nullptr if the pool page isn't storing a store page's data.
   */
  inline size_t page_id() const noexcept {
    DCHECK_NE(store_, nullptr);
    return page_id_;
  }

  /** The page data held by this page. */
  inline uint8_t* data() noexcept {
    return reinterpret_cast<uint8_t*>(this + 1);
  }

#if DCHECK_IS_ON()
  /** The pool that this page belongs to. Solely intended for use in DCHECKs. */
  inline const PagePool* page_pool() const noexcept { return page_pool_; }
#endif  // DCHECK_IS_ON

  /** True if the pool page's contents can be replaced. */
  inline bool IsUnpinned() const noexcept { return pin_count_ == 0; }

  /** Increments the page's pin count. */
  inline void AddPin() noexcept {
    DCHECK_NE(pin_count_, kMaxPinCount);
    ++pin_count_;
  }

  /** Decrements the page's pin count. */
  inline void RemovePin() noexcept {
    DCHECK(pin_count_ != 0);
    --pin_count_;
  }

  /** The next page in the list that the page belongs to. */
  inline Page* next() const noexcept { return next_; }

  /** The previous page in the list that the page belongs to. */
  inline Page* prev() const noexcept { return prev_; }

  /** True if this page is the lits head sentinel of an empty list.
   *
   * The return value is undefined if this page is not a list head sentinel.
   */
  inline bool IsEmptyListSentinel() const noexcept {
    DCHECK(is_sentinel_);
    // Redundant with the check above, might trigger if memory gets corrupted.
    DCHECK_EQ(store_, nullptr);
    DCHECK_EQ(pin_count_, kSentinelPinCount);
    DCHECK(next_ != nullptr);
    DCHECK(prev_ != nullptr);

    DCHECK_EQ(next_ == this, prev_ == this);

    // It might be tempting to check if the list's size is 0. For now, we only
    // track list sizes for performance monitoring, and don't want to rely on
    // the sizes for correctness.
    return next_ == this;
  }

  /** The number of pages in this list head sentinel's list.
   *
   * The return value is undefined if this page is not a list head sentinel. */
  inline size_t list_size() const noexcept {
    DCHECK(is_sentinel_);
    // Redundant with the check above, might trigger if memory gets corrupted.
    DCHECK_EQ(store_, nullptr);
    DCHECK_EQ(pin_count_, kSentinelPinCount);
    DCHECK(next_ != nullptr);
    DCHECK(prev_ != nullptr);

    return page_id_;
  }

  /** Removes a page from this list head sentinel's list.
   *
   * This page must be a list head sentinel.
   *
   * @param page the page to be removed; must not be a sentinel */
  inline void RemoveFromList(Page* page) noexcept {
    DCHECK(is_sentinel_);
    // Redundant with the check above, might trigger if memory gets corrupted.
    DCHECK_EQ(pin_count_, kSentinelPinCount);
    DCHECK(next_ != nullptr);
    DCHECK(prev_ != nullptr);

    DCHECK_EQ(page->page_pool_, page_pool_);
    DCHECK(!page->is_sentinel_);
    DCHECK(page->next_ != nullptr);
    DCHECK(page->prev_ != nullptr);

    --page_id_;  // page_id_ holds the list size in list sentinels.
    page->next_->prev_ = page->prev_;
    page->prev_->next_ = page->next_;

#if DCHECK_IS_ON()
    page->next_ = nullptr;
    page->prev_ = nullptr;
#endif  // DCHECK_IS_ON()
  }

  /** Inserts a page in this sentinel page's list.
   *
   * This page must be a list head sentinel.
   *
   * @param page the page to be inserted; must not be a sentinel, and must not
   *             aleady be in a list
   * @param page the page after which the new page will be inserted
   */
  inline void InsertPageAfter(Page* page, Page* after) {
    DCHECK(is_sentinel_);
    // Redundant with the check above, might trigger if memory gets corrupted.
    DCHECK_EQ(pin_count_, kSentinelPinCount);
    DCHECK(next_ != nullptr);
    DCHECK(prev_ != nullptr);

    DCHECK(!page->is_sentinel_);
    DCHECK_EQ(page_pool_, page->page_pool_);
    DCHECK_EQ(page->next_, nullptr);
    DCHECK_EQ(page->prev_, nullptr);

    DCHECK_EQ(page_pool_, after->page_pool_);
    DCHECK(after->next_ != nullptr);
    DCHECK(after->prev_ != nullptr);

    ++page_id_;  // page_id_ holds the list size in list sentinels.
    page->prev_ = after;
    page->next_ = after->next_;
    after->next_->prev_ = page;
    after->next_ = page;
  }

  /** Track the fact that the pool page will cache a store page.
   *
   * The page should not be in any list while a store page is loaded into it,
   * so Alloc() doesn't grab it. This also implies that the page must be pinned.
   */
  inline void AssignToStore(StoreImpl* store, size_t page_id) noexcept {
    DCHECK(!is_sentinel_);
    DCHECK(pin_count_ != 0);
    DCHECK_EQ(store_, nullptr);
    DCHECK_EQ(next_, nullptr);
    DCHECK_EQ(prev_, nullptr);
    // Attempt to Catch cases where the code flushing the page forgets to clear
    // the dirty flag.
    DCHECK(!is_dirty_);

    DCHECK_EQ(page_pool_, store->page_pool());

    store_ = store;
    page_id_ = page_id;
  }

  /** Track the fact that the pool page no longer caches a store page.
   *
   * The page must be pinned, as it was caching a store page up until now. This
   * also implies that the page cannot be on any list. */
  inline void UnassignFromStore() noexcept {
    DCHECK(!is_sentinel_);
    DCHECK(pin_count_ != 0);
    DCHECK(store_ != nullptr);
    DCHECK_EQ(next_, nullptr);
    DCHECK_EQ(prev_, nullptr);

#if DCHECK_IS_ON()
    store_ = nullptr;
#endif  // DCHECK_IS_ON()
  }

  /** The next page in the list that the page belongs to. */
  inline bool is_dirty() const noexcept {
    DCHECK(!is_sentinel_);
    return is_dirty_;
  }

  /** Changes the page's dirtiness status.
   *
   * The page must be assigned to store while its dirtiness is changed. */
  inline void MarkDirty(bool will_be_dirty = true) noexcept {
    DCHECK(store_ != nullptr);
    is_dirty_ = will_be_dirty;
  }

 private:
   /** Use Page::Create() to construct Page instances. */
   Page(PagePool* page, std::nullptr_t is_not_sentinel);

   /** pin_count_ value for page control info stubs used as list heads.
    *
    * The value is intended to result in a recognizable pattern in debuggers and
    * in stack traces.
    */
   static constexpr size_t kSentinelPinCount = 0x80000000;

#if DCHECK_IS_ON()
  /** The maximum value that pin_count_ can hold.
   *
   * Pages should always be pinned by a very small number of modules.
   * Excessively large pin counts indicate leaks.
   */
  static constexpr size_t kMaxPinCount = ~static_cast<size_t>(0);
#endif  // DCHECK_IS_ON()

  Page* next_;
  Page* prev_;
  StoreImpl* store_;
  size_t page_id_;

  /** Number of times the page was pinned. Very similar to a reference count. */
  size_t pin_count_;

  /** True if the page's data was modified since the page was read.
   *
   * This should only be true for pool pages that cache store pages. When a
   * dirty page is removed from the pool, its content must be written to disk.
   */
  bool is_dirty_;

#if DCHECK_IS_ON()
  PagePool* const page_pool_;
  bool const is_sentinel_;
#endif  // DCHECK_IS_ON()
};

}  // namespace berrydb

#endif  // BERRYDB_PAGE_H_
