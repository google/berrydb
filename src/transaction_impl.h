// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_TRANSACTION_IMPL_H_
#define BERRYDB_TRANSACTION_IMPL_H_

#include "berrydb/transaction.h"
#include "./page.h"
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

  /** Create a TransactionImpl instance. */
  static TransactionImpl* Create(StoreImpl* store);

  /** Computes the internal representation for a pointer from the public API. */
  static inline TransactionImpl* FromApi(Transaction* api) noexcept {
    TransactionImpl* impl = reinterpret_cast<TransactionImpl*>(api);
    DCHECK_EQ(api, &impl->api_);
    return impl;
  }
  /** Computes the internal representation for a pointer from the public API. */
  static inline const TransactionImpl* FromApi(
      const Transaction* api) noexcept {
    const TransactionImpl* impl = reinterpret_cast<const TransactionImpl*>(api);
    DCHECK_EQ(api, &impl->api_);
    return impl;
  }

  /** Computes the public API representation for this transaction. */
  inline Transaction* ToApi() noexcept { return &api_; }

  /** The store this transaction is running against. */
  inline StoreImpl* store() const noexcept { return store_; }

#if DCHECK_IS_ON()
  /** Number of pool pages assigned to this transaction. DCHECKs use only.
   *
   * This includes pinned pages and pages in the LRU list. */
  inline size_t AssignedPageCount() const noexcept {
    return pool_pages_.size();
  }
#endif  // DCHECK_IS_ON()

  /** Called when a Page is assigned to this transaction.
   *
   * Registers the page on the transaction's list of assigned pages, so the page
   * can be logged (for write transactions) or unassigned (for store init
   * transactions) when the transaction is closed.
   *
   * Must be called after the Page's transaction is set to this transaction. */
  inline void PageAssigned(Page* page) noexcept {
    DCHECK(page != nullptr);
    DCHECK_EQ(page->transaction(), this);
#if DCHECK_IS_ON()
    DcheckPageBelongsToTransaction(page);
#endif  // DCHECK_IS_ON()
    pool_pages_.push_back(page);
  }

  /** Called when a Page is unassigned from this transaction.
   *
   * Calls to this method must be paired with PageAssigned() calls. The call

   * Must be called before the Page's transaction is unset. */
  inline void PageUnassigned(Page* page) noexcept {
    DCHECK(page != nullptr);
#if DCHECK_IS_ON()
    DcheckPageBelongsToTransaction(page);
#endif  // DCHECK_IS_ON()

    pool_pages_.erase(page);
  }

  // See the public API documention for details.
  Status Get(Space* space, string_view key, string_view* value);
  Status Put(Space* space, string_view key, string_view value);
  Status Delete(Space* space, string_view key);
  Status Commit();
  Status Rollback();
  Status CreateSpace(
      CatalogImpl* catalog, string_view name, SpaceImpl** result);
  Status CreateCatalog(
      CatalogImpl* catalog, string_view name, CatalogImpl** result);
  Status Delete(CatalogImpl* catalog, string_view name);

  inline bool IsClosed() const noexcept {
    DCHECK(!is_committed_ || is_closed_);
    return is_closed_;
  }
  inline bool IsCommitted() const noexcept {
    DCHECK(!is_committed_ || is_closed_);
    return is_committed_;
  }
  /** True if the transaction was rolled back. */
  inline bool IsRolledBack() const noexcept {
    DCHECK(!is_committed_ || is_closed_);
    return is_closed_ && !is_committed_;
  }
  void Release();

 private:
  /** Use TransactionImpl::Create() to obtain TransactionImpl instances. */
  TransactionImpl(StoreImpl* store);

  /** Common path of commit and abort. */
  Status Close();

#if DCHECK_IS_ON()
  /** DCHECKs that the given page pool entry was assigned to this transaction.
   *
   * This method performs state consistency checks. The declaration is clunky,
   * but necessary because some of the checks depend on StoreImpl's definition,
   * and this file cannot include store_impl.h because StoreImpl contains a
   * TransactionImpl. */
  void DcheckPageBelongsToTransaction(Page* page);
#endif  // DCHECK_IS_ON()

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

#if DCHECK_IS_ON()
  /** True if this is the store's init transaction. */
  bool is_init_;
#endif  // DCHECK_IS_ON()
};

}  // namespace berrydb

#endif  // BERRYDB_TRANSACTION_IMPL_H_
