// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "berrydb/store.h"

#include "../catalog_impl.h"
#include "../store_impl.h"
#include "../transaction_impl.h"

namespace berrydb {

std::string Store::LogFilePath(const std::string &store_path) {
  return StoreImpl::LogFilePath(store_path);
}

Transaction* Store::CreateTransaction() {
  return StoreImpl::FromApi(this)->CreateTransaction()->ToApi();
}

Catalog* Store::RootCatalog() {
  return StoreImpl::FromApi(this)->RootCatalog()->ToApi();
}

Status Store::Close() {
  return StoreImpl::FromApi(this)->Close();
}

bool Store::IsClosed() {
  return StoreImpl::FromApi(this)->IsClosed();
}

void Store::Release() {
  StoreImpl::FromApi(this)->Release();
}

}  // namespace berrydb
