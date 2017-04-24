// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "./transaction_impl.h"

#include "./store_impl.h"

namespace berrydb {

static_assert(std::is_standard_layout<TransactionImpl>::value,
    "TransactionImpl must be a standard layout type so its public API can be "
    "exposed cheaply");

TransactionImpl* TransactionImpl::Create(StoreImpl* store) {
  void* heap_block = Allocate(sizeof(TransactionImpl));
  TransactionImpl* transaction = new (heap_block) TransactionImpl(store);
  DCHECK_EQ(heap_block, static_cast<void*>(transaction));
  return transaction;
}

TransactionImpl::TransactionImpl(StoreImpl* store) : store_(store) {
  DCHECK(store != nullptr);
}

Status Get(Space* space, string_view key, string_view* value) {
  return Status::kIoError;
}

Status Put(Space* space, string_view key, string_view value) {
  return Status::kIoError;
}

Status Delete(Space* space, string_view key) {
  return Status::kIoError;
}

Status Commit() {
  return Status::kIoError;
}

Status Abort() {
  return Status::kIoError;
}

}  // namespace berrydb
