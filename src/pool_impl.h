// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_POOL_IMPL_H_
#define BERRYDB_POOL_IMPL_H_

#include <functional>
#include <memory>
#include <tuple>
#include <unordered_set>

#include "./page_pool.h"
#include "./util/platform_allocator.h"
#include "berrydb/pool.h"

namespace berrydb {

class StoreImpl;
class Vfs;

/** Internal representation for the Pool class in the public API. */
class PoolImpl final : public Pool {
 public:
  // See the public API documention for details.
  static std::unique_ptr<PoolImpl> Create(const PoolOptions& options);
  PoolImpl(const PoolOptions& options, PassKey);
  ~PoolImpl() override;

  PoolImpl(const PoolImpl&) = delete;
  PoolImpl(PoolImpl&&) = delete;
  PoolImpl& operator=(const PoolImpl&) = delete;
  PoolImpl& operator=(PoolImpl&&) = delete;

  /** Invokes the platform allocator. */
  static void* operator new(size_t instance_size);

  /** This resource pool's page pool. */
  inline constexpr PagePool* page_pool() noexcept { return &page_pool_; }

  // See the public API documention for details.
  std::tuple<Status, Store*> OpenStore(const std::string& path,
                                       const StoreOptions& options) override;
  size_t PageSize() const noexcept override;
  size_t PagePoolSize() const noexcept override;

  /** Called upon the creation of a Store instance that uses this pool. */
  void StoreCreated(StoreImpl* store);

  /** Called when a Store that uses this pool is closed. */
  void StoreClosed(StoreImpl* store);

 private:
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
