// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "./free_page_manager.h"

#include "berrydb/platform.h"
#include "./free_page_list.h"
#include "./page.h"
#include "./page_pool.h"
#include "./store_impl.h"
#include "./transaction_impl.h"
#include "./util/checks.h"

namespace berrydb {

static_assert(
    FreePageManager::kInvalidPageId == FreePageList::kInvalidPageId,
    "kInvalidPageId must be the same in FreePageManager and FreePageList");

FreePageManager::FreePageManager(MAYBE_UNUSED StoreImpl* store)
#if BERRYDB_CHECK_IS_ON()
    : store_(store)
#endif  // BERRYDB_CHECK_IS_ON()
    {
  BERRYDB_ASSUME(store != nullptr);
}

FreePageManager::~FreePageManager() = default;

size_t FreePageManager::AllocPage(
    MAYBE_UNUSED TransactionImpl* transaction,
    MAYBE_UNUSED TransactionImpl* alloc_transaction) {
#if BERRYDB_CHECK_IS_ON()
  BERRYDB_CHECK_EQ(store_, transaction->store());
  BERRYDB_CHECK_EQ(store_, alloc_transaction->store());
#endif  // BERRYDB_CHECK_IS_ON()

  // TODO(pwnall): Check for free pages scoped to the transaction.

  // TODO(pwnall): Check for free pages scoped to the store.
  // TODO(pwnall): Write the new header to the db using alloc_transaction.

  return kInvalidPageId;
}

Status FreePageManager::FreePage(
    MAYBE_UNUSED size_t page_id, MAYBE_UNUSED TransactionImpl *transaction,
    MAYBE_UNUSED TransactionImpl *alloc_transaction) {

  return Status::kIoError;
}

}  // namespace berrydb
