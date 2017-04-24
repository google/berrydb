// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_INCLUDE_POOL_H_
#define BERRYDB_INCLUDE_POOL_H_

#include <string>

namespace berrydb {

class Store;

/** Options used to create a resource pool. */
struct PoolOptions {
  /** The base-2 logarithm of the pool's page size.
   *
   * The pool's page size can be computed as (1 << page_shift), or
   * 2 ** page_shift. The pool can only be used to open stores whose page size
   * matches the pool's page size.
   */
  size_t page_shift;

  /** Maximum number of store pages cached the page pool.
   *
   * The page pool's peak memory usage is bounded by the page size and the
   * maximum number of pages. Each page requires a small bookkeeping overhead.
   */
  size_t page_pool_size;
};

/** A pool of resources that can be shared among stores. */
class Pool {
 public:
  Store* Open(std::string path);

  /** Construct a new pool. */
  static Pool* Create(const PoolOptions& options);

  /** The store page size supported by this resource pool. */
  size_t page_size() const;

  /** The maximum number of store pages cached by the page pool. */
  size_t page_pool_size() const;
 private:
  friend class PoolImpl;

  /** Use Pool::Create() to create Pool instances. */
  Pool();
  /** Use Pool::Release() to destroy Pool instances. */
  ~Pool();
};

}  // namespace berrydb

#endif  // BERRYDB_INCLUDE_POOL_H_
