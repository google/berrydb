// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "./catalog_impl.h"

#include "berrydb/platform.h"
#include "berrydb/status.h"

namespace berrydb {

static_assert(std::is_standard_layout<CatalogImpl>::value,
    "CatalogImpl must be a standard layout type so its public API can be "
    "exposed cheaply");

CatalogImpl* CatalogImpl::Create() {
  void* heap_block = Allocate(sizeof(CatalogImpl));
  CatalogImpl* catalog = new (heap_block) CatalogImpl();
  DCHECK_EQ(heap_block, static_cast<void*>(catalog));
  return catalog;
}

CatalogImpl::CatalogImpl() = default;

CatalogImpl::~CatalogImpl() = default;

void CatalogImpl::Release() {
  this->~CatalogImpl();
  void* heap_block = static_cast<void*>(this);
  Deallocate(heap_block, sizeof(CatalogImpl));
}

std::tuple<Status, CatalogImpl*> CatalogImpl::OpenCatalog(
    span<const uint8_t> name) {
  UNUSED(name);
  return {Status::kIoError, nullptr};
}

std::tuple<Status, SpaceImpl*> CatalogImpl::OpenSpace(
    span<const uint8_t> name) {
  UNUSED(name);
  return {Status::kIoError, nullptr};
}

}  // namespace berrydb
