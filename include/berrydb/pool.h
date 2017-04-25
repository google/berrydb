// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_INCLUDE_POOL_H_
#define BERRYDB_INCLUDE_POOL_H_

#include <string>

#include "berrydb/status.h"

namespace berrydb {

struct PoolOptions;
struct StoreOptions;
class Store;

/** A pool of resources that can be shared among stores.
 *
 * Resource pools capture the bulk of a store's resource (memory, I/O) usage.
 * For best results, a system should have very few pools (ideally, one) that all
 * the stores use.
 */
class Pool {
 public:
  /** Construct a new resource pool. */
  static Pool* Create(const PoolOptions& options);

  /** Releases all resources held by this pool.
   *
   * This closes all the databases opened using this resource pool. */
  void Release();

  /** Open (or create) a store. */
  Status OpenStore(
      const std::string& path, const StoreOptions& options, Store** result);

  /** The store page size supported by this resource pool. */
  size_t page_size() const;

  /** The maximum number of store pages cached by the page pool. */
  size_t page_pool_size() const;

 private:
  friend class PoolImpl;

  /** Use Pool::Create() to create Pool instances. */
  Pool() = default;
  /** Use Release() to destroy Pool instances. */
  ~Pool() = default;
};

}  // namespace berrydb

#endif  // BERRYDB_INCLUDE_POOL_H_
