// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_INCLUDE_STORE_H_
#define BERRYDB_INCLUDE_STORE_H_

#include "berrydb/platform.h"

namespace berrydb {

enum class Status : int;

class Catalog;
class Transaction;

/** A key-value store. */
class Store {
 public:
  /** Starts a transaction against this store. */
  Transaction* CreateTransaction();

  /** Obtains the root catalog for this store.
   *
   * The root catalog is implicitly released when the Store is released, and
   * Release() should never be called on it explicitly. */
  Catalog* RootCatalog();

  /** Closes the store. */
  Status Close();

  /** True if the store is closed, false if it can still be used. */
  bool IsClosed();

  /** Releases the store's memory.
   *
   * Closes the store, if it hasn't been already closed. */
  void Release();
 private:
  friend class StoreImpl;

  /** Use Pool::OpenStore() to create Store instances. */
  Store() = default;
  /** Use Release() to destroy Store instances. */
  ~Store() = default;
};

}  // namespace berrydb

#endif  // BERRYDB_INCLUDE_STORE_H_
