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

namespace berrydb {

static_assert(
    FreePageManager::kInvalidPageId == FreePageList::kInvalidPageId,
    "kInvalidPageId must be the same in FreePageManager and FreePageList");

FreePageManager::FreePageManager(StoreImpl* store) : store_(store) {
}

FreePageManager::~FreePageManager() {
}

size_t FreePageManager::AllocPage(
    TransactionImpl* transaction, TransactionImpl* alloc_transaction) {
  DCHECK_EQ(store_, transaction->store());
  DCHECK_EQ(store_, alloc_transaction->store());

  UNUSED(store_);
  UNUSED(transaction);
  UNUSED(alloc_transaction);

  // TODO(pwnall): Check for free pages scoped to the transaction.

  // TODO(pwnall): Check for free pages scoped to the store.
  // TODO(pwnall): Write the new header to the db using alloc_transaction.

  return kInvalidPageId;
}

Status FreePageManager::FreePage(
    size_t page_id, TransactionImpl *transaction,
    TransactionImpl *alloc_transaction) {

  UNUSED(store_);
  UNUSED(page_id);
  UNUSED(transaction);
  UNUSED(alloc_transaction);

  return Status::kIoError;
}

}  // namespace berrydb
