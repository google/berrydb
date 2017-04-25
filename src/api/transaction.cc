// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "berrydb/transaction.h"

#include "../transaction_impl.h"

namespace berrydb {

Status Transaction::Get(Space* space, string_view key, string_view* value) {
  return TransactionImpl::FromApi(this)->Get(space, key, value);
}

Status Transaction::Put(Space* space, string_view key, string_view value) {
  return TransactionImpl::FromApi(this)->Put(space, key, value);
}

Status Transaction::Delete(Space* space, string_view key) {
  return TransactionImpl::FromApi(this)->Delete(space, key);
}

Status Transaction::Commit() {
  return TransactionImpl::FromApi(this)->Commit();
}

Status Transaction::Rollback() {
  return TransactionImpl::FromApi(this)->Rollback();
}

bool Transaction::IsClosed() {
  return TransactionImpl::FromApi(this)->IsClosed();
}

bool Transaction::IsCommitted() {
  return TransactionImpl::FromApi(this)->IsCommitted();
}

bool Transaction::IsRolledBack() {
  return TransactionImpl::FromApi(this)->IsRolledBack();
}

void Transaction::Release() {
  TransactionImpl::FromApi(this)->Release();
}

}  // namespace berrydb
