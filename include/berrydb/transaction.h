// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_INCLUDE_TRANSACTION_H_
#define BERRYDB_INCLUDE_TRANSACTION_H_

#include "berrydb/platform.h"

namespace berrydb {

enum class Status;

/**
 * An atomic, isolated (read committed) and durable unit of database operations.
 *
 * BerryDB transactions see the results of other committed transactions, which
 * is equivalent to the "read committed" SQL isolation level. This limited
 * guarantee was chosen because it is sufficient for implementing IndexedDB
 * transactions.
 */
class Transaction {
 public:
  /**
   * Reads a database key. Sees Put()s and Delete()s made by this transaction.
   *
   * @param  key   must point to valid memory until the call returns
   * @param  value guaranteed to point to valid memory until the next call on
   *               this transaction
   * @return       kNotFound if the key does not exist in the database
   */
  Status Get(string_view key, string_view& value);

  /**
   * Creates / updates a database key. Seen by Gets() made by this transaction.
   *
   * @param  key   must point to valid memory until the call returns
   * @param  value must point to valid memory until the call returns
   * @return       kSuccess, unless something bad happened
   */
  Status Put(string_view key, string_view value);

  /**
   * Creates / updates a database key. Seen by Gets() made by this transaction.
   *
   * @param  key   must point to valid memory until the call returns
   * @return       kSuccess, unless something bad happened
   */
  Status Delete(string_view key);

  /**
   * Exposes Put()s and Deletes() in this transaction to all other transactions.
   *
   * After this method is called, the transaction becomes invalid. No other
   * methods should be called.
   *
   * @return       kSuccess, unless something bad happened
   */
  Status Commit();

  /**
   * Discards the Put()s and Deletes() in this transaction.
   *
   * After this method is called, the transaction becomes invalid. No other
   * methods should be called.
   *
   * @return       kSuccess, unless something bad happened
   */
  Status Abort();

};

}  // namespace berrydb

#endif  // BERRYDB_INCLUDE_TRANSACTION_H_
