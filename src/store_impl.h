// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_STORE_IMPL_H_
#define BERRYDB_STORE_IMPL_H_

#include <functional>
#include <unordered_set>

#include "berrydb/platform.h"
#include "berrydb/pool.h"
#include "berrydb/store.h"
#include "berrydb/vfs.h"
#include "./util/linked_list.h"

namespace berrydb {

class BlockAccessFile;
class CatalogImpl;
class Page;
class PagePool;
class TransactionImpl;

/** Internal representation for the Store class in the public API. */
class StoreImpl {
 public:
  /** Create a StoreImpl instance. */
  static StoreImpl* Create(
      BlockAccessFile* data_file, PagePool* page_pool,
      const StoreOptions& options);

  /** Computes the internal representation for a pointer from the public API. */
  static inline StoreImpl* FromApi(Store* api) noexcept {
    StoreImpl* impl = reinterpret_cast<StoreImpl*>(api);
    DCHECK_EQ(api, &impl->api_);
    return impl;
  }
  /** Computes the internal representation for a pointer from the public API. */
  static inline const StoreImpl* FromApi(const Store* api) noexcept {
    const StoreImpl* impl = reinterpret_cast<const StoreImpl*>(api);
    DCHECK_EQ(api, &impl->api_);
    return impl;
  }

  /** Computes the public API representation for this store. */
  inline Store* ToApi() noexcept { return &api_; }

  // See the public API documention for details.
  TransactionImpl* CreateTransaction();
  inline CatalogImpl* RootCatalog() noexcept { return nullptr; }
  Status Close();
  inline bool IsClosed() const noexcept { return state_ == State::kClosed; }
  void Release();

  /** Reads a page from the store into the page pool.
   *
   * The page pool entry must have already been assigned to store, and must not
   * be holding onto a dirty page.
   *
   * @param  page the page pool entry that will hold the store's page;
   * @return      most likely kSuccess or kIoError */
  Status ReadPage(Page* page);

  /** Writes a page to the store.
   *
   * @param  page the page pool entry caching the store page to be written
   * @return      most likely kSuccess or kIoError */
  Status WritePage(Page* page);

  /** Updates the store to reflect a transaction's commit / abort.
   *
   * @param transaction must be associated with this store, and closed */
  void TransactionClosed(TransactionImpl* transaction);

#if DCHECK_IS_ON()
  /** The page pool used by this store. For use in DCHECKs only. */
  inline PagePool* page_pool() const noexcept { return page_pool_; }
#endif  // DCHECK_IS_ON()

 private:
  /** Use StoreImpl::Create() to obtain StoreImpl instances. */
  StoreImpl(
      BlockAccessFile* data_file, PagePool* page_pool,
      const StoreOptions& options);
  /** Use Release() to destroy StoreImpl instances. */
  ~StoreImpl();

  enum class State {
    kOpen = 0,
    kClosing = 1,
    kClosed = 2,
  };

  /* The public API version of this class. */
  Store api_;  // Must be the first class member.

  /** Handle to the store's data file. */
  BlockAccessFile* const data_file_;

  /** The page pool used by this store to interact with its data file. */
  PagePool* const page_pool_;

  /** The base-2 logarithm of the store's page size. */
  const size_t page_shift_;

  /** The transactions opened on this store. */
  LinkedList<TransactionImpl> transactions_;

  State state_ = State::kOpen;
};

}  // namespace berrydb

#endif  // BERRYDB_STORE_IMPL_H_
