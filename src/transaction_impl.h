// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_TRANSACTION_IMPL_H_
#define BERRYDB_TRANSACTION_IMPL_H_

#include <tuple>

#include "berrydb/span.h"
#include "berrydb/transaction.h"
#include "./page.h"
// #include "./page_pool.h" would cause a cycle
// #include "./store_impl.h" would cause a cycle
#include "./util/checks.h"
#include "./util/linked_list.h"

namespace berrydb {

class BlockAccessFile;
class CatalogImpl;
class SpaceImpl;
class StoreImpl;
class TransactionImpl;

/** Internal representation for the Transaction class in the public API.
 *
 * A transaction is associated with a store for its entire lifecycle. For
 * resource cleanup purposes, each store has a linked list of all its live
 * transactinons. To reduce dynamic memory allocations, the linked list nodes
 * are embedded in the transaction objects.
 */
class TransactionImpl {
 public:
  /** Constructor for a store's init transaction. */
  TransactionImpl(StoreImpl* store, bool is_init);
  /** Public destructor needed for store init transactions.
   *
   * Use Release() to destroy instances created by TransactionImpl::Create(). */
  ~TransactionImpl();

  TransactionImpl(const TransactionImpl&) = delete;
  TransactionImpl(TransactionImpl&&) = delete;
  TransactionImpl& operator=(const TransactionImpl&) = delete;
  TransactionImpl& operator=(TransactionImpl&&) = delete;

  /** Create a TransactionImpl instance. */
  static TransactionImpl* Create(StoreImpl* store);

  /** Computes the internal representation for a pointer from the public API. */
  static inline TransactionImpl* FromApi(Transaction* api) noexcept {
    TransactionImpl* const impl = reinterpret_cast<TransactionImpl*>(api);
    BERRYDB_ASSUME_EQ(api, &impl->api_);
    return impl;
  }
  /** Computes the internal representation for a pointer from the public API. */
  static inline const TransactionImpl* FromApi(
      const Transaction* api) noexcept {
    const TransactionImpl* const impl =
        reinterpret_cast<const TransactionImpl*>(api);
    BERRYDB_ASSUME_EQ(api, &impl->api_);
    return impl;
  }

  /** Computes the public API representation for this transaction. */
  inline constexpr Transaction* ToApi() noexcept { return &api_; }

  /** The store this transaction is running against. */
  inline constexpr StoreImpl* store() const noexcept { return store_; }

#if BERRYDB_CHECK_IS_ON()
  /** Number of pool pages assigned to this transaction. CHECKs use only.
   *
   * This includes pinned pages and pages in the LRU list. */
  inline constexpr size_t AssignedPageCount() const noexcept {
    return pool_pages_.size();
  }

  /** True if this transaction is the store's init transaction. CHECKs use only.
   *
   * This method is not allowed for non-CHECK use because its implementation
   * cannot be inlined. The implementation cannot be inlined because it depends
   * on the StoreImpl class, whose declaration depends on TransactionImpl. */
  bool IsInit() const noexcept;
#endif  // BERRYDB_CHECK_IS_ON()

  /** Prepares a page pool entry for caching a page in this transaction's store.
   *
   * Registers the page on the transaction's list of assigned pages, so the page
   * can be logged (for write transactions) or unassigned (for store init
   * transactions) when the transaction is closed.
   *
   * The caller must have a pin on the page pool entry, and the page pool entry
   * must not be currently assigned to cache any store's data.
   *
   * @param page    the page pool entry that will cache a store page
   * @param page_id the ID of the store data page that will be cached by the
   *                page pool entry
   */
  inline void AssignPage(Page* page, size_t page_id) noexcept {
    BERRYDB_ASSUME(page != nullptr);
    BERRYDB_ASSUME(!page->IsUnpinned());
    BERRYDB_ASSUME(page->transaction() == nullptr);

    page->WillCacheStoreData(this, page_id);
    pool_pages_.push_back(page);
  }

  /** Prepares a Page that will not be caching a page in this transaction store.
   *
   * The caller must have a pin on the page pool entry. The page pool entry must
   * be currently caching a page in this transaction's store, and must be
   * assigned to this transaction.
   *
   * @param page a page pool entry that was caching a page in this transaction's
   *             store, and will not be caching the page anymore
   */
  inline void UnassignPage(Page* page) noexcept {
    BERRYDB_ASSUME(page != nullptr);
    BERRYDB_ASSUME(!page->IsUnpinned());
    BERRYDB_ASSUME_EQ(page->transaction(), this);
#if BERRYDB_CHECK_IS_ON()
    CheckPageBelongsToTransaction(page);
#endif  // BERRYDB_CHECK_IS_ON()

    pool_pages_.erase(page);
    page->DoesNotCacheStoreData();
  }

  /** Called when this transaction will modify a page pool entry's data buffer.
   *
   * In order to guarantee durability, this method must be called before the
   * entry's data buffer is modified. The modification is tied to this
   * transaction, and will be persisted iff the transaction is committed.
   *
   * This page pool entry must be caching a page that belongs to the
   * transaction's store. The caller must have a pin on this page pool entry.
   *
   * @param page the Page whose data buffer will be modified in this transaction
   */
  inline void WillModifyPage(Page* page) noexcept {
    BERRYDB_ASSUME(page != nullptr);
    BERRYDB_ASSUME(!page->IsUnpinned());
    BERRYDB_ASSUME(page->transaction() != nullptr);
    BERRYDB_ASSUME_EQ(page->transaction()->store(), store_);

// The init transaction will never be committed, so it cannot be used to
// modify pages.
#if BERRYDB_CHECK_IS_ON()
    BERRYDB_CHECK(!is_init_);
#endif  // BERRYDB_CHECK_IS_ON()

    TransactionImpl* const page_transaction = page->transaction();
    if (page_transaction != this) {
// A page may not be modified by two transactions at the same time. This
// follows from the concurrency model, which states that a Space modified
// by a transaction must not be accessed by any concurrent transaction.
#if BERRYDB_CHECK_IS_ON()
      BERRYDB_CHECK(page->transaction()->is_init_);
#endif  // BERRYDB_CHECK_IS_ON()
      BERRYDB_ASSUME(!page->is_dirty());

      // TODO(pwnall): Once logging is done, consider if it's possible for a
      //     page not to be dirty while it is assigned to a non-init
      //     transaction. If not, this check can be turned into an early return
      //     when the page is already assigned to this transaction.
      page_transaction->pool_pages_.erase(page);
      pool_pages_.push_back(page);
      page->ReassignToTransaction(this);
    }

    page->SetDirty(true);
  }

  /** Called when a page assigned to this transaction was persisted.
   *
   * Pages should only be persisted when they are dirty. Persisting a page
   * involves writing it to the store data file, or writing a REDO log record
   * for it to the log file, and (in some cases) flushing the log file.
   *
   * @param page the Page whose data buffer was written to persistent storage
   */
  inline void PageWasPersisted(Page* page,
                               TransactionImpl* init_transaction) noexcept {
    BERRYDB_ASSUME(page != nullptr);
    BERRYDB_ASSUME(!page->IsUnpinned());
    BERRYDB_ASSUME_EQ(page->transaction(), this);

    BERRYDB_ASSUME(init_transaction != nullptr);
    BERRYDB_ASSUME_EQ(init_transaction->store(), store_);
#if BERRYDB_CHECK_IS_ON()
    BERRYDB_CHECK(init_transaction->is_init_);
#endif  // BERRYDB_CHECK_IS_ON()

    if (this == init_transaction) {
      // Pages owned by an init transaction cannot be dirty.
      BERRYDB_ASSUME(!page->is_dirty());
      return;
    }

    pool_pages_.erase(page);
    init_transaction->pool_pages_.push_back(page);
    page->ReassignToTransaction(init_transaction);
    page->SetDirty(false);
  }

  /** Prepares a Page that will not be caching a page in this transaction store.
   *
   * The caller must have a pin on the page pool entry. The page pool entry must
   * be currently caching a page in this transaction's store, and must be
   * assigned to this transaction. The page must have been recently
   *
   * @param page a page pool entry that was caching a page in this transaction's
   *             store, and will not be caching the page anymore
   */
  inline void UnassignPersistedPage(Page* page) noexcept {
    BERRYDB_ASSUME(page != nullptr);
    BERRYDB_ASSUME(!page->IsUnpinned());
    BERRYDB_ASSUME_EQ(page->transaction(), this);
    BERRYDB_ASSUME(page->is_dirty());
#if BERRYDB_CHECK_IS_ON()
    BERRYDB_CHECK(!is_init_);
#endif  // BERRYDB_CHECK_IS_ON()

    pool_pages_.erase(page);
    page->DoesNotCacheStoreData();
    page->SetDirty(false);
  }

  // See the public API documention for details.
  std::tuple<Status, span<const uint8_t>> Get(SpaceImpl* space,
                                              span<const uint8_t> key);
  Status Put(SpaceImpl* space, span<const uint8_t> key,
             span<const uint8_t> value);
  Status Delete(SpaceImpl* space, span<const uint8_t> key);
  Status Commit();
  Status Rollback();
  std::tuple<Status, SpaceImpl*> CreateSpace(CatalogImpl* catalog,
                                             span<const uint8_t> name);
  std::tuple<Status, CatalogImpl*> CreateCatalog(CatalogImpl* catalog,
                                                 span<const uint8_t> name);
  Status Delete(CatalogImpl* catalog, span<const uint8_t> name);

  inline constexpr bool IsClosed() const noexcept {
    BERRYDB_ASSUME(!is_committed_ || is_closed_);
    return is_closed_;
  }
  inline constexpr bool IsCommitted() const noexcept {
    BERRYDB_ASSUME(!is_committed_ || is_closed_);
    return is_committed_;
  }
  /** True if the transaction was rolled back. */
  inline constexpr bool IsRolledBack() const noexcept {
    BERRYDB_ASSUME(!is_committed_ || is_closed_);
    return is_closed_ && !is_committed_;
  }
  void Release();

 private:
  /** Use TransactionImpl::Create() to obtain TransactionImpl instances. */
  TransactionImpl(StoreImpl* store);

  /** Common functionality in Commit() and Rollback(). */
  Status Close();

#if BERRYDB_CHECK_IS_ON()
  /** CHECKs that the given page pool entry was assigned to this transaction.
   *
   * This method performs state consistency checks. The declaration is clunky,
   * but necessary because some of the checks depend on StoreImpl's definition,
   * and this file cannot include store_impl.h because StoreImpl contains a
   * TransactionImpl. */
  void CheckPageBelongsToTransaction(Page* page);
#endif  // BERRYDB_CHECK_IS_ON()

  /* The public API version of this class. */
  Transaction api_;  // Must be the first class member.

  friend class LinkedListBridge<TransactionImpl>;
  LinkedList<TransactionImpl>::Node linked_list_node_;

  /** Entries in the page pool whose buffers were modified by this transaction.
   *
   * When the transaction commits, the pages on this list need REDO log records.
   *
   * Store init transactions use this list to track all the pages in the pool
   * that are assigned to the store, but are not on a transaction's list. This
   * means the init transaction's list will contain the page pool entries
   * assigned to the store that have not been modified by an ongoing
   * transaction, or have already been written back to the store.
   */
  LinkedList<Page, Page::TransactionLinkedListBridge> pool_pages_;

  /** The store this transaction runs against. */
  StoreImpl* const store_;

  bool is_closed_ = false;
  bool is_committed_ = false;

#if BERRYDB_CHECK_IS_ON()
  /** True if this is the store's init transaction. */
  bool is_init_;
#endif  // BERRYDB_CHECK_IS_ON()
};

}  // namespace berrydb

#endif  // BERRYDB_TRANSACTION_IMPL_H_
