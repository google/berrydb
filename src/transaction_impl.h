// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_TRANSACTION_IMPL_H_
#define BERRYDB_TRANSACTION_IMPL_H_

#include "berrydb/transaction.h"
#include "./util/linked_list.h"

namespace berrydb {

class BlockAccessFile;
class CatalogImpl;
class SpaceImpl;
class StoreImpl;

/** Internal representation for the Transaction class in the public API.
 *
 * A transaction is associated with a store for its entire lifecycle. For
 * resource cleanup purposes, each store has a linked list of all its live
 * transactinons. To reduce dynamic memory allocations, the linked list nodes
 * are embedded in the transaction objects.
 */
class TransactionImpl {
 public:
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

#if DCHECK_IS_ON()
  /** The store this transaction is running against. For use in DCHECKs only. */
  inline StoreImpl* store() const noexcept { return store_; }
#endif  // DCHECK_IS_ON()

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
  /** Use Release() to destroy TransactionImpl instances. */
  ~TransactionImpl();

  /** Common path of commit and abort. */
  Status Close();

  /* The public API version of this class. */
  Transaction api_;  // Must be the first class member.

  friend class LinkedListBridge<TransactionImpl>;
  LinkedList<TransactionImpl>::Node linked_list_node_;

  /** The store this transaction runs against. */
  StoreImpl* const store_;

  bool is_closed_ = false;
  bool is_committed_ = false;
};

}  // namespace berrydb

#endif  // BERRYDB_TRANSACTION_IMPL_H_
