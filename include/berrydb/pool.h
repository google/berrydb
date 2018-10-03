// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_INCLUDE_BERRYDB_POOL_H_
#define BERRYDB_INCLUDE_BERRYDB_POOL_H_

#include <memory>
#include <string>
#include <tuple>

#include "berrydb/types.h"

namespace berrydb {

struct PoolOptions;
enum class Status : int;
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
  // Access control for the Pool constructor.
  class PassKey {
   private:
    constexpr PassKey() noexcept {}

    friend class Pool;
    friend class PoolImpl;
  };

  /** Construct a new resource pool. */
  static std::unique_ptr<Pool> Create(const PoolOptions& options);

  /** Use Pool::Create() to create Pool instances.
   *
   * This constructor is public for use by std::make_unique. */
  constexpr Pool(PassKey) noexcept {}

  /** Releases all resources held by this pool.
   *
   * This closes all the databases opened using this resource pool. */
  virtual ~Pool() noexcept = default;

  /** Invokes the platform allocator. */
  static void operator delete(void* instance, size_t instance_size);

  /** Open (or create) a store. */
  virtual std::tuple<Status, Store*> OpenStore(const std::string& path,
                                               const StoreOptions& options) = 0;

  /** The store page size supported by this resource pool. */
  virtual size_t PageSize() const noexcept = 0;

  /** The maximum number of store pages cached by the page pool. */
  virtual size_t PagePoolSize() const noexcept = 0;
};

}  // namespace berrydb

#endif  // BERRYDB_INCLUDE_BERRYDB_POOL_H_
