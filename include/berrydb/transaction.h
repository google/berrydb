// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_INCLUDE_TRANSACTION_H_
#define BERRYDB_INCLUDE_TRANSACTION_H_

#include "berrydb/platform.h"

namespace berrydb {

class Catalog;
class Space;
enum class Status : int;

/**
 * An atomic and durable (once committed) unit of database operations.
 *
 * Transactions should use an external synchronization mechanism to avoid
 * read-write conflicts. Specifically, if a transaction writes to a catalog or
 * to a (key-value name)space, its lifetime must not overlap with the lifetime
 * of any other transaction that either reads from or writes to the same catalog
 * or space.
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
  Status Rollback();

  /** Creates a (key/value name)space.
   *
   * @param catalog will store the reference to the new (key/value name)space
   * @param name    the name of the newly created namespace
   * @param result  if the operation succeeds, receives a pointer to the newly
   *                created catalog
   * @return        kAlreadyExists if the catalog already contains an entry with
   *                the given name; otherwise, most likely kSuccess or kIoError
   */
  Status CreateSpace(Catalog* catalog, string_view name, Space** result);

  /** Creates a catalog.
   *
   * @param catalog will store the reference to the new catalog
   * @param name    the name of the newly created namespace
   * @param result  if the operation succeeds, receives a pointer to the newly
   *                created space
   * @return        kAlreadyExists if the catalog already contains an entry with
   *                the given name; otherwise, most likely kSuccess or kIoError
   */
  Status CreateCatalog(Catalog* catalog, string_view name, Catalog** result);

  /** Deletes a (key/value name)space and all its content, or a catalog.
   *
   * @param catalog the catalog that stores the reference to the catalog or the
   *                (key/value name)space that will be deleted
   * @param name    the name of the catalog or (key/value name)space that will
   *                be deleted
   * @return        kNotFound if the catalog does not contain an entry with the
   *                given name; otherwise, most likely kSuccess or kIoError
   */
  Status Delete(Catalog* catalog, string_view name);

  /** True if the transaction was committed or rolled back. */
  bool IsClosed();

  /** True if the transaction was committed. */
  bool IsCommitted();

  /** True if the transaction was rolled back. */
  bool IsRolledBack();

  /** Releases the transaction's memory.
   *
   * If the transaction is in progress, it is rolled back. */
  void Release();

 private:
  friend class TransactionImpl;

  /** Use Store::CreateTransaction() to create Transaction instances. */
  constexpr Transaction() noexcept = default;
  /** Use Release() to destroy Transaction instances. */
  ~Transaction() noexcept = default;
};

}  // namespace berrydb

#endif  // BERRYDB_INCLUDE_TRANSACTION_H_
