// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "berrydb/catalog.h"

#include "berrydb/status.h"
#include "../catalog_impl.h"
#include "../space_impl.h"

namespace berrydb {

void Catalog::Release() {
  return CatalogImpl::FromApi(this)->Release();
}

std::tuple<Status, Catalog*> Catalog::OpenCatalog(span<const uint8_t> name) {
  Status status;
  CatalogImpl* catalog;
  std::tie(status, catalog) = CatalogImpl::FromApi(this)->OpenCatalog(name);
  return {status, catalog->ToApi()};
}

std::tuple<Status, Space*> Catalog::OpenSpace(span<const uint8_t> name) {
  Status status;
  SpaceImpl* space;
  std::tie(status, space) = CatalogImpl::FromApi(this)->OpenSpace(name);
  return {status, space->ToApi()};
}

}  // namespace berrydb
