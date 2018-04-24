// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "berrydb/transaction.h"

#include "berrydb/status.h"
#include "../catalog_impl.h"
#include "../space_impl.h"
#include "../transaction_impl.h"

namespace berrydb {

std::tuple<Status, span<const uint8_t>> Transaction::Get(
    Space* space, span<const uint8_t> key) {
  return TransactionImpl::FromApi(this)->Get(SpaceImpl::FromApi(space), key);
}

Status Transaction::Put(Space* space, span<const uint8_t> key,
                        span<const uint8_t> value) {
  return TransactionImpl::FromApi(this)->Put(SpaceImpl::FromApi(space), key,
                                             value);
}

Status Transaction::Delete(Space* space, span<const uint8_t> key) {
  return TransactionImpl::FromApi(this)->Delete(SpaceImpl::FromApi(space), key);
}

Status Transaction::Commit() {
  return TransactionImpl::FromApi(this)->Commit();
}

Status Transaction::Rollback() {
  return TransactionImpl::FromApi(this)->Rollback();
}

std::tuple<Status, Space*> Transaction::CreateSpace(
    Catalog* catalog, span<const uint8_t> name) {
  Status status;
  SpaceImpl* space;
  std::tie(status, space) = TransactionImpl::FromApi(this)->CreateSpace(
      CatalogImpl::FromApi(catalog), name);
  return {status, space->ToApi()};
}

std::tuple<Status, Catalog*> Transaction::CreateCatalog(
    Catalog* catalog, span<const uint8_t> name) {
  Status status;
  CatalogImpl* new_catalog;
  std::tie(status, new_catalog) = TransactionImpl::FromApi(this)->CreateCatalog(
     CatalogImpl::FromApi(catalog), name);
  return {status, new_catalog->ToApi()};
}

Status Transaction::Delete(Catalog* catalog, span<const uint8_t> name) {
  return TransactionImpl::FromApi(this)->Delete(
      CatalogImpl::FromApi(catalog), name);
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
