// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_INCLUDE_BERRYDB_STORE_H_
#define BERRYDB_INCLUDE_BERRYDB_STORE_H_

#include <string>

namespace berrydb {

enum class Status : int;

class Catalog;
class Transaction;

/**
 * An autonomous unit of storage, holding a tree of key-value stores.
 *
 * A store has a distinct physical representation on disk as a collection of
 * files. Stores can be used and deleted independently of each other. Corruption
 * in a store's data can impact the entire store's content, but will not impact
 * other stores.
 *
 * A store is made up of spaces, which store user data. "Space" stands for
 * key-value namespace, and is a mapping (function) from strings to strings. In
 * other words, a space can have at most one value associated with a given key.
 *
 * For the purposes here, "string" is used in the C++ sense of std::string, so a
 * string is actually a sequence of bytes, not a sequence of (most likely
 * Unicode) codepoints or characters.
 *
 * Spaces are connected by catalogs. A catalog is a key-value namespace where
 * the keys are strings and the values are either stores or catalogs. A catalog
 * is similar to a directory in a file-system, which can be seen as is a
 * collection of named references to other directories (catalogs) or files
 * (spaces).
 *
 * In other words, a store is a tree whose inner nodes are catalogs, and whose
 * leaves are spaces. Edges are labeled using strings, and all the edges
 * connected a node to its children have different labels.
 */
class Store {
 public:
  /** The path of the log file associated with a store file.
   *
   * This method allocates a string using the libc allocator instead of
   * PlatformAllocator. In this case, we consider that opens are infrequent
   * and cause small allocations, so routing them through PlatformAllocator
   * isn't worth the code overhead. This decision should be revisited whenever
   * another std::string allocation is added to the codebase.
   *
   * @return a file path that can be passed to Vfs::OpenForRandomAccess() to
   *         open the store's log file; the log file is not guaranteed to exist
   */
  static std::string LogFilePath(const std::string& store_path);

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
  constexpr Store() noexcept = default;
  /** Use Release() to destroy Store instances. */
  ~Store() noexcept = default;
};

}  // namespace berrydb

#endif  // BERRYDB_INCLUDE_BERRYDB_STORE_H_
