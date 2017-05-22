// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "./transaction_impl.h"

#include "berrydb/status.h"
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

void TransactionImpl::Release() {
  this->~TransactionImpl();
  void* heap_block = static_cast<void*>(this);
  Deallocate(heap_block, sizeof(TransactionImpl));
}

TransactionImpl::TransactionImpl(StoreImpl* store) : store_(store) {
  DCHECK(store != nullptr);
}
TransactionImpl::~TransactionImpl() {
  if (!is_closed_)
    Rollback();
}

Status TransactionImpl::Get(Space* space, string_view key, string_view* value) {
  if (is_closed_)
    return Status::kAlreadyClosed;

  UNUSED(space);
  UNUSED(key);
  UNUSED(value);
  return Status::kIoError;
}

Status TransactionImpl::Put(Space* space, string_view key, string_view value) {
  if (is_closed_)
    return Status::kAlreadyClosed;

  UNUSED(space);
  UNUSED(key);
  UNUSED(value);
  return Status::kIoError;
}

Status TransactionImpl::Delete(Space* space, string_view key) {
  if (is_closed_)
    return Status::kAlreadyClosed;

  UNUSED(space);
  UNUSED(key);
  return Status::kIoError;
}

Status TransactionImpl::Close() {
  DCHECK(!is_closed_);

  is_closed_ = true;
  store_->TransactionClosed(this);
  return Status::kSuccess;
}

Status TransactionImpl::Commit() {
  if (is_closed_)
    return Status::kAlreadyClosed;

  is_committed_ = true;
  return Close();
}

Status TransactionImpl::Rollback() {
  if (is_closed_)
    return Status::kAlreadyClosed;

  return Close();
}

Status TransactionImpl::CreateSpace(
    CatalogImpl* catalog, string_view name, SpaceImpl** result) {
  if (is_closed_)
    return Status::kAlreadyClosed;

  UNUSED(catalog);
  UNUSED(name);
  UNUSED(result);
  return Status::kIoError;

}

Status TransactionImpl::CreateCatalog(
    CatalogImpl* catalog, string_view name, CatalogImpl** result) {

  if (is_closed_)
    return Status::kAlreadyClosed;
  UNUSED(catalog);
  UNUSED(name);
  UNUSED(result);
  return Status::kIoError;
}

Status TransactionImpl::Delete(CatalogImpl* catalog, string_view name) {
  if (is_closed_)
    return Status::kAlreadyClosed;

  UNUSED(catalog);
  UNUSED(name);
  return Status::kIoError;
}

}  // namespace berrydb
