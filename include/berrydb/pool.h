// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

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
   * The page pool's memory usage will be
   */
  size_t page_pool_size;
};

/** A pool of resources that can be shared among stores. */
class Pool {
 public:
  Store* Open(std::string path);

  /** Construct a new pool. */
  static Pool* New(const PoolOptions& options);

  /** The store page size supported by this resource pool. */
  inline size_t page_size() const { return 1 << options_.page_shift; }

  /** The options used to create this pool. */
  inline const PoolOptions& options() const { return options_; }

 private:
  PoolOptions options_;
};


}  // namespace berrydb
