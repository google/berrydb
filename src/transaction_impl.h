// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_TRANSACTION_IMPL_H_
#define BERRYDB_TRANSACTION_IMPL_H_

#include "berrydb/transaction.h"

namespace berrydb {

class BlockAccessFile;
class StoreImpl;

/** Internal representation for the Transaction class in the public API. */
class TransactionImpl {
 public:
  /** Create a TransactionImpl instance. */
  static TransactionImpl* Create(StoreImpl* store);

  /** Computes the internal representation for a pointer from the public API. */
  static inline TransactionImpl* FromApi(Transaction* api) noexcept {
    TransactionImpl* transaction = reinterpret_cast<TransactionImpl*>(api);
    DCHECK_EQ(api, &transaction->api_);
    return transaction;
  }
  /** Computes the internal representation for a pointer from the public API. */
  static inline const TransactionImpl* FromApi(const Transaction* api) noexcept {
    const TransactionImpl* transaction =
        reinterpret_cast<const TransactionImpl*>(api);
    DCHECK_EQ(api, &transaction->api_);
    return transaction;
  }

  /** Computes the public API representation for this transaction. */
  inline Transaction* ToApi() noexcept { return &api_; }

  // See the public API documention for details.
  Status Get(Space* space, string_view key, string_view* value);
  Status Put(Space* space, string_view key, string_view value);
  Status Delete(Space* space, string_view key);
  Status Commit();
  Status Abort();

 private:
  /** Use TransactionImpl::Create() to obtain TransactionImpl instances. */
  TransactionImpl(StoreImpl* store);

  /* The public API version of this class. */
  Transaction api_;  // Must be the first class member.

  StoreImpl* const store_;
};

}  // namespace berrydb

#endif  // BERRYDB_TRANSACTION_IMPL_H_
