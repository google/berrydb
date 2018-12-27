// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_PAGE_H_
#define BERRYDB_PAGE_H_

#include <cstddef>
#include <cstdint>

#include "berrydb/span.h"
// #include "./store_impl.h" would cause a cycle
// #include "./transaction_impl.h" would cause a cycle
#include "./util/checks.h"
#include "./util/linked_list.h"

namespace berrydb {

class PagePool;
class StoreImpl;
class TransactionImpl;

/** Control block for a page pool entry, which caches a store page.
 *
 * Although this class represents a page pool entry, it is simply named Page,
 * because most of the system only cares about the store page cached into the
 * entry's buffer.
 *
 * Each entry in a page pool has a control block (the members of this class),
 * which is laid out in memory right before the buffer that holds the content of
 * the cached store page.
 *
 * An entry belongs to the same PagePool for its entire lifetime. The entry's
 * control block does not hold a reference to the pool (in release mode) to save
 * memory.
 *
 * Each page pool entry has a pin count, which works like a reference count.
 * While an entry is pinned (has at least one pin), it will not be evicted.
 * Conversely, unpinned entries may be evicted and assigned to cache different
 * store pages at any time.
 *
 * Most pages will be stored in a doubly linked list used to implement the LRU
 * eviction policy. To reduce memory allocations, the list nodes are embedded in
 * the page control block.
 *
 * Each linked list has a sentinel. For simplicity, the sentinel is simply a
 * page control block without a page data buffer.
 */
class Page {
 public:
  /** Allocates an entry that will belong to the given page pool.
   *
   * The returned page has one pin on it, which is owned by the caller. */
  static Page* Create(PagePool* page_pool);

  Page(const Page&) = delete;
  Page(Page&&) = delete;
  Page& operator=(const Page&) = delete;
  Page& operator=(Page&&) = delete;

  /** Releases the memory resources used up by this page pool entry.
   *
   * This method invalidates the Page instance, so it must not be used
   * afterwards. */
  void Release(PagePool* page_pool);

  /** The transaction that this page pool entry is assigned to.
   *
   * Each page pool entry that has been modified by an uncommitted transaction
   * is assigned to that transaction. Other page pool entries that cache a
   * store's pages are assigned to the store's init transaction. This
   * relationship is well defined because, according to the concurrency model,
   * no two concurrent transactions may modify the same Space, and each page
   * belongs at most one space.
   *
   * When CHECKs are enabled, this is null when the page is not assigned to a
   * transaction. When CHECKs are disabled, the value is undefined when the
   * page is not assigend to a transaction.
   */
  inline constexpr TransactionImpl* transaction() const noexcept {
    return transaction_;
  }

  /** The page ID of the store page whose data is cached by this pool page.
   *
   * This is undefined if the page pool entry isn't storing a store page's data.
   */
  inline constexpr size_t page_id() const noexcept {
    BERRYDB_ASSUME(transaction_ != nullptr);
    return page_id_;
  }

  /** True if the page's data was modified since the page was read.
   *
   * This should only be true for pool pages that cache store pages. When a
   * dirty page is removed from the pool, its content must be written to disk.
   */
  inline constexpr bool is_dirty() const noexcept {
    BERRYDB_ASSUME(!is_dirty_ || transaction_ != nullptr);
    return is_dirty_;
  }

  /** The page's data buffer.
   *
   * Prefer using data() when the page's size is readily computed. */
  inline const uint8_t* buffer() const noexcept {
    return reinterpret_cast<const uint8_t*>(this + 1);
  }

  /** The page's data buffer.
   *
   * The caller must own a pin to this page.
   *
   * Prefer using mutable_data() when the page's size is readily computed. */
  inline uint8_t* mutable_buffer() noexcept {
    return reinterpret_cast<uint8_t*>(this + 1);
  }

  /** An immutable reference to the page's data.
   *
   * @param  page_size must match the page size of the PagePool used to
   *                   construct this Page instance
   * @return an immutable span covering the page's data
   */
  inline span<const uint8_t> data(size_t page_size) const noexcept {
#if BERRYDB_CHECK_IS_ON()
    CheckPageSizeMatches(page_size);
#endif  // BERRYDB_CHECK_IS_ON()
    return span<const uint8_t>(buffer(), page_size);
  }

  /** The page's data.
   *
   * The caller must own a pin to the page.
   *
   * @param  page_size must match the page size of the PagePool used to
   *                   construct this Page instance
   * @return a mutable span covering the page's data
   */
  inline span<uint8_t> mutable_data(size_t page_size) noexcept {
    BERRYDB_ASSUME(!IsUnpinned());
#if BERRYDB_CHECK_IS_ON()
    CheckPageSizeMatches(page_size);
#endif  // BERRYDB_CHECK_IS_ON()
    return span<uint8_t>(mutable_buffer(), page_size);
  }

#if BERRYDB_CHECK_IS_ON()
  /** The pool that this page belongs to. Solely intended for use in CHECKs. */
  inline constexpr const PagePool* page_pool() const noexcept {
    return page_pool_;
  }
#endif  // BERRYDB_CHECK_IS_ON

  /** True if the pool page's contents can be replaced. */
  inline constexpr bool IsUnpinned() const noexcept {
    return pin_count_ == 0;
  }

  /** Increments the page's pin count. */
  inline void AddPin() noexcept {
    BERRYDB_ASSUME_NE(pin_count_, kMaxPinCount);
    ++pin_count_;
  }

  /** Decrements the page's pin count. */
  inline void RemovePin() noexcept {
    BERRYDB_ASSUME(pin_count_ != 0);
    --pin_count_;
  }

  /** Track the fact that the pool page entry will cache a store page.
   *
   * This method is exposed for use from TransactionImpl::AssignPage().
   *
   * The page should not be in any LRU list while a store page is loaded into
   * it, so PagePool::Alloc() doesn't grab it. This also implies that the page
   * must be pinned. */
  inline void WillCacheStoreData(TransactionImpl* transaction,
                                 size_t page_id) noexcept {
    BERRYDB_ASSUME(transaction != nullptr);
    BERRYDB_ASSUME(pin_count_ != 0);
    BERRYDB_ASSUME(!is_dirty_);
#if BERRYDB_CHECK_IS_ON()
    BERRYDB_CHECK(transaction_ == nullptr);
    BERRYDB_CHECK(transaction_list_node_.list_sentinel() == nullptr);
    BERRYDB_CHECK(linked_list_node_.list_sentinel() == nullptr);
    CheckTransactionAssignmentIsValid(transaction);
#endif  // BERRYDB_CHECK_IS_ON()

    transaction_ = transaction;
    page_id_ = page_id;
  }

  /** Track the fact that the pool page entry no longer caches a store page.
   *
   * This method is exposed for use from TransactionImpl::UnassignPage().
   *
   * This pool page entry must be pinned, so PagePool::Alloc() does not grab it.
   * The entry must have at most one pin, owned by the caller. Otherwise, the
   * other pin owners will have the page's data change unexpectedly.
   */
  inline void DoesNotCacheStoreData() noexcept {
    BERRYDB_ASSUME_EQ(pin_count_, 1U);
    BERRYDB_ASSUME(transaction_ != nullptr);
#if BERRYDB_CHECK_IS_ON()
    // Fails if TransactionImpl::PageWillBeUnassigned() was not called right
    // before calling this method.
    BERRYDB_ASSUME(transaction_list_node_.list_sentinel() == nullptr);
    BERRYDB_ASSUME(linked_list_node_.list_sentinel() == nullptr);
#endif  // BERRYDB_CHECK_IS_ON()

#if BERRYDB_CHECK_IS_ON()
    transaction_ = nullptr;
#endif  // BERRYDB_CHECK_IS_ON()
  }

  /** Dirty flag setter for PagePool and TransactionImpl.
   *
   * This should not be used directly in most cases. It is exposed for use by
   * PagePool and by TransactionImpl.
   *
   * @param is_dirty the new value of the page pool entry's dirty flag
   */
  inline void SetDirty(bool is_dirty) noexcept {
#if BERRYDB_CHECK_IS_ON()
    CheckNewDirtyValueIsValid(is_dirty);
#endif  // BERRYDB_CHECK_IS_ON()

    is_dirty_ = is_dirty;
  }

  /** Called when the Page is reassigned to a new transaction in the same store.
   *
   * This should not be used directly in most cases. It is exposed for use by
   * TransactionImpl. The calling code is responsible for updating the
   * LinkedList<Page> members for the impacted transactions.
   *
   * A page pool entry can only be reassigned to a transaction that belongs to
   * the same store. This implies that neither the current nor the new
   * transaction may be null. Either the current or the new transaction must be
   * the store's init transaction.
   *
   * @param transaction the transaction that the Page is reassigned to
   */
  inline void ReassignToTransaction(TransactionImpl* transaction) noexcept {
    BERRYDB_ASSUME(transaction != nullptr);
    BERRYDB_ASSUME(transaction_ != nullptr);
#if BERRYDB_CHECK_IS_ON()
    CheckTransactionReassignmentIsValid(transaction);
#endif  // BERRYDB_CHECK_IS_ON()

    transaction_ = transaction;
  }

 private:
  /** Use Page::Create() to construct Page instances. */
  Page(PagePool* page);
  ~Page();

  /** The maximum value that pin_count_ can hold.
   *
   * Pages should always be pinned by a very small number of modules.
   * Excessively large pin counts indicate leaks.
   */
  static constexpr size_t kMaxPinCount = ~static_cast<size_t>(0);

#if BERRYDB_CHECK_IS_ON()
  // These methods performs state consistency checks. The declarations are
  // clunky, but necessary because some of the checks depend on the definitions
  // of StoreImpl and TransactionImpl. This file cannot include their headers
  // because StoreImpl and TransactionImpl contain LinkedList<Page> fields,
  // and their inlined methods call into inlined Page methods.

  /** CHECKs that a transaction assignment for this page is valid.
   *
   * @param transaction the transaction that the Page will be assigned to */
  void CheckTransactionAssignmentIsValid(
      TransactionImpl* transaction) noexcept;

  /** CHECKs that the given dirty flag value makes sense for this page.
   *
   * @param is_dirty the new value of the dirty's page flag */
  void CheckNewDirtyValueIsValid(bool is_dirty) noexcept;

  /** CHECKs that a transaction reassignment for this page is valid.
   *
   * @param transaction the transaction that the Page will be reassigned to */
  void CheckTransactionReassignmentIsValid(
      TransactionImpl* transaction) noexcept;

  /** CHECKs that the page's size matches the given argument.
   *
   * @param page_size the size that this page must match */
  void CheckPageSizeMatches(size_t page_size) const noexcept;
#endif  // BERRYDB_CHECK_IS_ON()

  friend class LinkedListBridge<Page>;
  LinkedList<Page>::Node linked_list_node_;

  friend class TransactionLinkedListBridge;
  LinkedList<Page>::Node transaction_list_node_;

  /** The transaction this page is used by.
   *
   * When CHECKs are enabled, this is null when the page is not assigned to a
   * transaction. When CHECKs are disabled, the value is undefined when the
   * page is not assigend to a transaction.
   */
  TransactionImpl* transaction_;

  /** The cached page ID, for pool entries that are caching a store's pages.
   *
   * This member's memory is available for use (perhaps via an union) by
   * unassigned pages.
   */
  size_t page_id_;

  /** Number of times the page was pinned. Very similar to a reference count. */
  size_t pin_count_;
  bool is_dirty_ = false;

#if BERRYDB_CHECK_IS_ON()
  PagePool* const page_pool_;
#endif  // BERRYDB_CHECK_IS_ON()

 public:
  /** Bridge for TransactionImpl's LinkedList<Page>.
   *
   * This is public for TransactionImpl's use. */
  class TransactionLinkedListBridge {
   public:
    using Embedder = Page;
    using Node = LinkedListNode<Page>;

    static inline Node* NodeForHost(Embedder* host) noexcept {
      return &host->transaction_list_node_;
    }
    static inline Embedder* HostForNode(Node* node) noexcept {
      Embedder* const host = reinterpret_cast<Embedder*>(
          reinterpret_cast<char*>(node) -
          offsetof(Embedder, transaction_list_node_));
      BERRYDB_ASSUME_EQ(node, &host->transaction_list_node_);
      return host;
    }
  };
};

}  // namespace berrydb

#endif  // BERRYDB_PAGE_H_
