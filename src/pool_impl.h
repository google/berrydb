// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "berrydb/pool.h"
#include "./page_pool.h"

namespace berrydb {

/** Internal representation for the Pool class in the public API. */
class PoolImpl {
 public:
  /** Create a new pool. */
  static PoolImpl* Create(const PoolOptions& options);

  /** Computes the public API Pool* for this resource pool. */
  inline Pool* ToPool() noexcept { return &api_; }

  /** The store page size supported by this resource pool. */
  inline size_t page_size() const noexcept { return page_pool_.page_size(); }

  /** Maximum number of store pages cached the page pool. */
  inline size_t page_pool_size() const noexcept {
    return page_pool_.page_capacity();
  }

 private:
  /** Use PoolImpl::Create() to obtain PoolImpl instances. */
  PoolImpl(const PoolOptions& options);

  /** Computes the PoolImpl* for a Pool* coming from the public API. */
  static inline PoolImpl* FromPool(Pool* api) noexcept {
    PoolImpl* pool = reinterpret_cast<PoolImpl*>(api);
    DCHECK_EQ(api, &pool->api_);
    return pool;
  }
  /** Computes the PoolImpl* for a Pool* coming from the public API. */
  static inline const PoolImpl* FromPool(const Pool* api) noexcept {
    const PoolImpl* pool = reinterpret_cast<const PoolImpl*>(api);
    DCHECK_EQ(api, &pool->api_);
    return pool;
  }

  // So it can access FromPool() and ToPool().
  friend class Pool;

  /** The public part of this class must be its first member. */
  Pool api_;

  /** The page pool part of this resource pool. */
  PagePool page_pool_;
};

}  // namespace berrydb
