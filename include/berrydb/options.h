// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_INCLUDE_BERRYDB_OPTIONS_H_
#define BERRYDB_INCLUDE_BERRYDB_OPTIONS_H_

#include "berrydb/types.h"

namespace berrydb {

class Vfs;

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

  /** The platform services implementation used by the resource pool.
   *
   * All the stores that use the resource pool must perform their operations via
   * the same VFS instance.
   *
   * If nullptr is specified, berrydb::DefaultVfs() is used to obtain the pool's
   * VFS.
   */
  Vfs* vfs;

  /** Defaults. */
  PoolOptions();
};

/** Options used to create a store. */
struct StoreOptions {
  /** If false, opening a non-existent store will fail. */
  bool create_if_missing;

  /** If true, opening an existent store will fail.
   *
   * If this option is true, create_if_missing must also be true. */
  bool error_if_exists;

  /** Defaults. */
  StoreOptions();
};

}  // namespace berrydb

#endif  // BERRYDB_INCLUDE_BERRYDB_OPTIONS_H_
