// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_POOL_IMPL_H_
#define BERRYDB_POOL_IMPL_H_

#include <functional>
#include <tuple>
#include <unordered_set>

#include "./page_pool.h"
#include "./util/platform_allocator.h"
#include "berrydb/pool.h"

namespace berrydb {

class StoreImpl;
class Vfs;

/** Internal representation for the Pool class in the public API. */
class PoolImpl {
 public:
  // See the public API documention for details.
  static PoolImpl* Create(const PoolOptions& options);

  /** Computes the PoolImpl* for a Pool* coming from the public API. */
  static inline PoolImpl* FromApi(Pool* api) noexcept {
    PoolImpl* impl = reinterpret_cast<PoolImpl*>(api);
    DCHECK_EQ(api, &impl->api_);
    return impl;
  }
  /** Computes the PoolImpl* for a Pool* coming from the public API. */
  static inline const PoolImpl* FromApi(const Pool* api) noexcept {
    const PoolImpl* impl = reinterpret_cast<const PoolImpl*>(api);
    DCHECK_EQ(api, &impl->api_);
    return impl;
  }

  /** Computes the public API Pool* for this resource pool. */
  inline Pool* ToApi() noexcept { return &api_; }

  /** This resource pool's page pool. */
  inline PagePool* page_pool() noexcept { return &page_pool_; }

  // See the public API documention for details.
  void Release();
  std::tuple<Status, StoreImpl*> OpenStore(const std::string& path,
                                           const StoreOptions& options);
  inline size_t page_size() const noexcept { return page_pool_.page_size(); }
  inline size_t page_pool_size() const noexcept {
    return page_pool_.page_capacity();
  }

  /** Called upon the creation of a Store instance that uses this pool. */
  void StoreCreated(StoreImpl* store);

  /** Called when a Store that uses this pool is closed. */
  void StoreClosed(StoreImpl* store);

 private:
  /** Use PoolImpl::Create() to obtain PoolImpl instances. */
  PoolImpl(const PoolOptions& options);
  /** Use Release() to delete PoolImpl instances. */
  ~PoolImpl();

  /* The public API version of this class. */
  Pool api_;  // Must be the first class member.

  /** The page pool part of this resource pool. */
  PagePool page_pool_;

  /** The opened stores that use this resource pool. */
  using StoreSet = std::unordered_set<StoreImpl*,
                                      PointerHasher<StoreImpl>,
                                      std::equal_to<StoreImpl*>,
                                      PlatformAllocator<StoreImpl*>>;
  StoreSet stores_;

  /** The platform services implementation used by this pool's stores. */
  Vfs* const vfs_;
};

}  // namespace berrydb

#endif  // BERRYDB_POOL_IMPL_H_
