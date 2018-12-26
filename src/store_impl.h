// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_STORE_IMPL_H_
#define BERRYDB_STORE_IMPL_H_

#include <functional>
#include <unordered_set>

#include "./format/store_header.h"
#include "./page.h"
#include "berrydb/platform.h"
#include "berrydb/pool.h"
#include "berrydb/store.h"
#include "berrydb/vfs.h"
// #include "./page_pool.h" would cause a cycle
#include "./transaction_impl.h"
#include "./util/linked_list.h"

namespace berrydb {

class BlockAccessFile;
class CatalogImpl;
class PagePool;
class PoolImpl;

/** Internal representation for the Store class in the public API. */
class StoreImpl {
 public:
  /** Public for testing convenience. */
  enum class State {
    kOpen = 0,
    kClosing = 1,
    kClosed = 2,
  };

  /** Create a StoreImpl instance.
   *
   * This returns a minimally set up instance that can be registered with the
   * pool. The new instance should be initalized via StoreImpl::Initialize()
   * before it is used for transactions.
   */
  static StoreImpl* Create(BlockAccessFile* data_file,
                           size_t data_file_size,
                           RandomAccessFile* log_file,
                           size_t log_file_size,
                           PagePool* page_pool,
                           const StoreOptions& options);

  StoreImpl(const StoreImpl&) = delete;
  StoreImpl(StoreImpl&&) = delete;
  StoreImpl& operator=(const StoreImpl&) = delete;
  StoreImpl& operator=(StoreImpl&&) = delete;

  /** Computes the internal representation for a pointer from the public API. */
  static inline StoreImpl* FromApi(Store* api) noexcept {
    StoreImpl* const impl = reinterpret_cast<StoreImpl*>(api);
    BERRYDB_ASSUME_EQ(api, &impl->api_);
    return impl;
  }
  /** Computes the internal representation for a pointer from the public API. */
  static inline const StoreImpl* FromApi(const Store* api) noexcept {
    const StoreImpl* const impl = reinterpret_cast<const StoreImpl*>(api);
    BERRYDB_ASSUME_EQ(api, &impl->api_);
    return impl;
  }

  /** Computes the public API representation for this store. */
  inline constexpr Store* ToApi() noexcept { return &api_; }

  /** The store's init transaction.
   *
   * This method is not const because callers may need to modify the init
   * transaction. For example, assigning a page to a transaction inserts it into
   * a linked list, and therefore modifies the transaction. */
  inline constexpr TransactionImpl* init_transaction() noexcept {
    return &init_transaction_;
  }

  /** The page pool used by this store. */
  inline constexpr PagePool* page_pool() const noexcept { return page_pool_; }

  // See the public API documention for details.
  static std::string LogFilePath(const std::string& store_path);
  TransactionImpl* CreateTransaction();
  inline constexpr CatalogImpl* RootCatalog() noexcept { return nullptr; }
  Status Close();
  inline constexpr bool IsClosed() const noexcept {
    return state_ == State::kClosed;
  }
  void Release();

  /** Initializes a store obtained by Store::Create.
   *
   * Store::Create() gets the store to a state where it can honor the Close()
   * call, so it can be registered with its resource pool. Before the store can
   * process user transactions, it must be initialized using this method.
   *
   * This method writes the initial on-disk data structures for new stores, and
   * kicks off the recovery process for existing stores. Therefore, it is quite
   * possible that initialization will fail due to an I/O error. Callers should
   * be prepared to handle the error.
   */
  Status Initialize(const StoreOptions& options);

  /** Builds a new store on the currently opened files. */
  Status Bootstrap();

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
   * The page pool entry must be flagged as dirty. The caller is responsible for
   * clearing the page entry's dirty flag if this method succeeds.
   *
   * @param  page the page pool entry caching the store page to be written
   * @return      most likely kSuccess or kIoError */
  Status WritePage(Page* page);

  /** Updates the store to reflect a transaction's commit / roll back.
   *
   * @param transaction must be associated with this store, and closed */
  void TransactionClosed(TransactionImpl* transaction);

#if BERRYDB_CHECK_IS_ON()
  /** Number of pool pages assigned to this store. For use in CHECKs only.
   *
   * This includes pinned pages and pages in the LRU list. */
  size_t AssignedPageCount() noexcept;
#endif  // BERRYDB_CHECK_IS_ON()

 private:
  /** Use StoreImpl::Create() to obtain StoreImpl instances. */
  StoreImpl(BlockAccessFile* data_file,
            size_t data_file_size,
            RandomAccessFile* log_file,
            size_t log_file_size,
            PagePool* page_pool,
            const StoreOptions& options);
  /** Use Release() to destroy StoreImpl instances. */
  ~StoreImpl();

  /* The public API version of this class. */
  Store api_;  // Must be the first class member.

  /** Handle to the store's data file. */
  BlockAccessFile* const data_file_;

  /** Handle to the store's log file. */
  RandomAccessFile* const log_file_;

  /** The page pool used by this store to interact with its data file. */
  PagePool* const page_pool_;

  /** The transactions opened on this store. */
  LinkedList<TransactionImpl> transactions_;

  /** The store's init transaction.
   *
   * Each store has a transaction that plays a similar role to the init process
   * (PID 0) in a UNIX system. The transaction is used to create the store's
   * initial pages, and owns the page pool entries that are assigned to the
   * store, but are not tracked by another transaction.
   */
  TransactionImpl init_transaction_;

  /** Metadata in the data file's header. */
  StoreHeader header_;

  State state_ = State::kOpen;
};

}  // namespace berrydb

#endif  // BERRYDB_STORE_IMPL_H_
