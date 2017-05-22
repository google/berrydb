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

Status Catalog::OpenCatalog(string_view name, Catalog** result) {
  CatalogImpl* catalog;
  Status status = CatalogImpl::FromApi(this)->OpenCatalog(name, &catalog);
  if (status == Status::kSuccess)
    *result = catalog->ToApi();
  return status;
}

Status Catalog::OpenSpace(string_view name, Space** result) {
  SpaceImpl* space;
  Status status = CatalogImpl::FromApi(this)->OpenSpace(name, &space);
  if (status == Status::kSuccess)
    *result = space->ToApi();
  return status;
}

}  // namespace berrydb
