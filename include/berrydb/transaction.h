// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_INCLUDE_BERRYDB_TRANSACTION_H_
#define BERRYDB_INCLUDE_BERRYDB_TRANSACTION_H_

#include <tuple>

#include "berrydb/span.h"
#include "berrydb/types.h"

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
  std::tuple<Status, span<const uint8_t>> Get(Space* space,
                                              span<const uint8_t> key);

  /** Creates / updates a store key. Seen by Gets() made by this transaction. */
  Status Put(Space* space, span<const uint8_t> key, span<const uint8_t> value);

  /** Deletes a store key. Seen by Gets() made by this transaction. */
  Status Delete(Space* space, span<const uint8_t> key);

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
   * @param catalog  the parent of the newly created namespace
   * @param name     the name of the newly created namespace
   * @return status  kAlreadyExists if the catalog already contains an entry
   *                 with the given name; otherwise, most likely kSuccess or
   *                 kIoError
   * @return catalog the newly created (key/value name)space
   */
  std::tuple<Status, Space*> CreateSpace(Catalog* catalog,
                                         span<const uint8_t> name);

  /** Creates a catalog.
   *
   * @param catalog  the parent of the newly created catalog
   * @param name     the name of the newly created catalog
   * @return status  kAlreadyExists if the catalog already contains an entry
   *                 with the given name; otherwise, most likely kSuccess or
   *                 kIoError
   * @return catalog the newly created catalog
   */
  std::tuple<Status, Catalog*> CreateCatalog(Catalog* catalog,
                                             span<const uint8_t> name);

  /** Deletes a (key/value name)space and all its content, or a catalog.
   *
   * @param catalog the catalog that stores the reference to the catalog or the
   *                (key/value name)space that will be deleted
   * @param name    the name of the catalog or (key/value name)space that will
   *                be deleted
   * @return        kNotFound if the catalog does not contain an entry with the
   *                given name; otherwise, most likely kSuccess or kIoError
   */
  Status Delete(Catalog* catalog, span<const uint8_t> name);

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

#endif  // BERRYDB_INCLUDE_BERRYDB_TRANSACTION_H_
