// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_PAGE_H_
#define BERRYDB_PAGE_H_

#include <cstddef>
#include <cstdint>

#include "berrydb/platform.h"
#include "./util/linked_list.h"

namespace berrydb {

class PagePool;
class StoreImpl;

/** Control block for a page pool entry, which can hold a store page in memory.
 *
 * Each page in a page pool has a control block (this class), which is laid out
 * in memory right before the buffer that holds the page data.
 *
 * A page belongs to the same PagePool for its entire lifetime. The page does
 * not hold a reference to the pool (in release mode) to save space.
 *
 * Each page has a pin count. This is very similar to a reference count. While a
 * page is pinned, its contents cannot be replaced. Therefore, pinned pages
 * must not be in the pool's LRU queue.
 *
 * Most pages will be stored in a doubly linked list used to implement the LRU
 * eviction policy. To reduce memory allocations, the list nodes are embedded in
 * the page control block.
 *
 * Each linked list has a sentinel. For simplicity, the sentinel is simply a
 * page control block without a page data buffer.
 */
class Page {
  enum class Status;

 public:
  /** Allocates a page that will belong to the given pool.
   *
   * The returned page has one pin on it, which is owned by the caller. */
  static Page* Create(PagePool* page_pool);

  /** Releases the memory resources used up by this page.
   *
   * The page must not be a list head sentinel. This method invalidates the Page
   * instance, so it must not be used afterwards. */
  void Release(PagePool* page_pool);

  /** The store whose data is cached by this page.
   *
   * When DCHECKs are enabled, this is null when the page is not assigned to a
   * store. When DCHECKs are disabled, the value is undefined when the page is
   * not assigend to a store.
   */
  inline StoreImpl* store() const noexcept {
    // It is tempting to DCHECK that store_ is not nullptr, to make sure this
    // getter isn't called when its value is undefined. However, DCHECKed builds
    // _can_ call this when the page is not assigned to a store, for the purpose
    // of higher level DCHECKs.
    return store_;
  }

  /** The page ID of the store page whose data is cached by this pool page.
   *
   * This is undefined if the page pool entry isn't storing a store page's data.
   */
  inline size_t page_id() const noexcept {
    DCHECK(store_ != nullptr);
    return page_id_;
  }

  /** True if the page's data was modified since the page was read.
   *
   * This should only be true for pool pages that cache store pages. When a
   * dirty page is removed from the pool, its content must be written to disk.
   */
  inline bool is_dirty() const noexcept {
    DCHECK(!is_dirty_ || store_ != nullptr);
    return is_dirty_;
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
#if DCHECK_IS_ON()
    DCHECK_NE(pin_count_, kMaxPinCount);
#endif  // DCHECK_IS_ON
    ++pin_count_;
  }

  /** Decrements the page's pin count. */
  inline void RemovePin() noexcept {
    DCHECK(pin_count_ != 0);
    --pin_count_;
  }

  /** Track the fact that the pool page will cache a store page.
   *
   * The page should not be in any list while a store page is loaded into it,
   * so Alloc() doesn't grab it. This also implies that the page must be pinned.
   *
   * The caller must immediately call StoreImpl::PageAssigned(). This method
   * cannot make that call, due to header dependencies.
   */
  inline void AssignToStore(StoreImpl* store, size_t page_id) noexcept {
    // NOTE: It'd be nice to DCHECK_EQ(page_pool_, store->page_pool()).
    //       Unfortunately, that requires a dependency on store_impl.h, which
    //       absolutely needs to include page.h.
#if DCHECK_IS_ON()
    DCHECK(store_ == nullptr);
    DCHECK(store_list_node_.list_sentinel() == nullptr);
    DCHECK(linked_list_node_.list_sentinel() == nullptr);
#endif  // DCHECK_IS_ON()
    DCHECK(pin_count_ != 0);
    DCHECK(!is_dirty_);

    store_ = store;
    page_id_ = page_id;
  }

  /** Track the fact that the pool page no longer caches a store page.
   *
   * The page must be pinned, as it was caching a store page up until now. This
   * also implies that the page cannot be on any list.
   *
   * The caller must immediately call StoreImpl::PageUnassigned(). This method
   * cannot make that call, due to header dependencies.
   */
  inline void UnassignFromStore() noexcept {
    DCHECK(pin_count_ != 0);
    DCHECK(store_ != nullptr);
#if DCHECK_IS_ON()
    DCHECK(store_list_node_.list_sentinel() != nullptr);
    DCHECK(linked_list_node_.list_sentinel() == nullptr);
#endif  // DCHECK_IS_ON()

#if DCHECK_IS_ON()
    store_ = nullptr;
#endif  // DCHECK_IS_ON()
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
   Page(PagePool* page);

#if DCHECK_IS_ON()
  /** The maximum value that pin_count_ can hold.
   *
   * Pages should always be pinned by a very small number of modules.
   * Excessively large pin counts indicate leaks.
   */
  static constexpr size_t kMaxPinCount = ~static_cast<size_t>(0);
#endif  // DCHECK_IS_ON()

  friend class LinkedListBridge<Page>;
  LinkedList<Page>::Node linked_list_node_;

  friend class StoreLinkedListBridge;
  LinkedList<Page>::Node store_list_node_;

  StoreImpl* store_;
  size_t page_id_;

  /** Number of times the page was pinned. Very similar to a reference count. */
  size_t pin_count_;
  bool is_dirty_ = false;

#if DCHECK_IS_ON()
  PagePool* const page_pool_;
#endif  // DCHECK_IS_ON()

 public:
  /** Bridge for StoreImpl's LinkedList<Page>. Exposed for StoreImpl. */
  class StoreLinkedListBridge {
   public:
    using Embedder = Page;
    using Node = LinkedListNode<Page>;

    static inline Node* NodeForHost(Embedder* host) noexcept {
      return &host->store_list_node_;
    }
    static inline Embedder* HostForNode(Node* node) noexcept {
      Embedder* host = reinterpret_cast<Embedder*>(
          reinterpret_cast<char*>(node) - offsetof(Embedder, store_list_node_));
      DCHECK_EQ(node, &host->store_list_node_);
      return host;
    }
  };
};

}  // namespace berrydb

#endif  // BERRYDB_PAGE_H_
