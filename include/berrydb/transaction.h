// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_INCLUDE_TRANSACTION_H_
#define BERRYDB_INCLUDE_TRANSACTION_H_

#include "berrydb/platform.h"

namespace berrydb {

class Space;
enum class Status : int;

/**
 * An atomic and durable (once committed) unit of database operations.
 *
 * Transactions should use an external synchronization mechanism to avoid
 * read-write conflicts. Specifically, if a transaction writes to a Space, its
 * lifetime must not overlap with the lifetime of any other transaction that
 * either reads from or writes to the same Space.
 */
class Transaction {
 public:
  /** Reads a store key. Sees Put()s and Delete()s made by this transaction. */
  Status Get(Space* space, string_view key, string_view* value);

  /** Creates / updates a store key. Seen by Gets() made by this transaction. */
  Status Put(Space* space, string_view key, string_view value);

  /** Deletes a store key. Seen by Gets() made by this transaction. */
  Status Delete(Space* space, string_view key);

  /**
   * Writes Put()s and Deletes() in this transaction to durable storage.
   *
   * After this method is called, the transaction becomes invalid. No other
   * methods should be called.
   */
  Status Commit();

  /**
   * Discards the Put()s and Deletes() in this transaction.
   *
   * After this method is called, the transaction becomes invalid. No other
   * methods should be called.
   */
  Status Abort();
};

}  // namespace berrydb

#endif  // BERRYDB_INCLUDE_TRANSACTION_H_
