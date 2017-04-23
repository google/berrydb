// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_PAGE_H_
#define BERRYDB_PAGE_H_

#include <cstddef>
#include <cstdint>

#include "berrydb/platform.h"

namespace berrydb {

class PagePool;
class Store;

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
  /** Allocates a page that will belong to the given pool. */
  static Page* Create(PagePool* page_pool);

  /** Creates a stub page that can be used as a list sentinel. */
  Page(PagePool* page_pool);

  /** The store whose data is cached by this page. */
  inline const Store* store() const noexcept { return store_; }

  /** The page ID of the store page whose data is cached by this page. */
  inline size_t page_id() const noexcept { return page_id_; }

  /** The page data held by this page. */
  inline void* data() noexcept {
    return reinterpret_cast<void*>(this + 1);
  }

  /** True if the pool page's contents can be replaced. */
  inline bool IsUnpinned() const noexcept { return pin_count_ == 0; }

  /** Increments the page's pin count. */
  inline void Pin() noexcept {
    DCHECK_NE(pin_count_, kMaxPinCount);
    ++pin_count_;
  }

  /** Decrements the page's pin count. */
  inline void Unpin() noexcept {
    DCHECK(pin_count_ != 0);
    --pin_count_;
  }

  /** The next page in the list that the page belongs to. */
  inline Page* next() const noexcept { return next_; }

  /** The previous page in the list that the page belongs to. */
  inline Page* prev() const noexcept { return prev_; }

  /** Removes this page from the list that it belongs to. */
  inline void RemoveFromList() noexcept {
    DCHECK(next_ != nullptr);
    DCHECK(prev_ != nullptr);
    next_->prev_ = prev_;
    prev_->next_ = next_;
  }

  /** Inserts this page in the same list as a given page, right after it. */
  inline void InsertAfter(Page* node) {
    DCHECK_EQ(pool_, node->pool_);
    DCHECK_EQ(next_, nullptr);
    DCHECK_EQ(prev_, nullptr);
    prev_ = node;
    next_ = node->next_;
    node->next_ = this;
  }

#if DCHECK_IS_ON()
  /** The pool that this page belongs to. Solely intended for use in DCHECKs. */
  inline const PagePool* pool() const noexcept { return pool_; }
#endif  // DCHECK_IS_ON

 private:
   /** Use Page::Create() to construct Page instances. */
   Page();

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
  Store* store_;
  size_t page_id_;

  /** Number of times the page was pinned. */
  size_t pin_count_;

#if DCHECK_IS_ON()
  PagePool* pool_;
#endif  // DCHECK_IS_ON()
};

}  // namespace berrydb

#endif  // BERRYDB_PAGE_H_
