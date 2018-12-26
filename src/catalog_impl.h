// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_CATALOG_IMPL_H_
#define BERRYDB_CATALOG_IMPL_H_

#include "berrydb/catalog.h"
#include "berrydb/span.h"
#include "berrydb/types.h"
#include "./util/checks.h"

namespace berrydb {

class SpaceImpl;

/** Internal representation for the Catalog class in the public API. */
class CatalogImpl {
 public:
  static CatalogImpl* Create();

  CatalogImpl(const CatalogImpl&) = delete;
  CatalogImpl(CatalogImpl&&) = delete;
  CatalogImpl& operator=(const CatalogImpl&) = delete;
  CatalogImpl& operator=(CatalogImpl&&) = delete;

  /** Computes the internal representation for a pointer from the public API. */
  static inline CatalogImpl* FromApi(Catalog* api) noexcept {
    CatalogImpl* impl = reinterpret_cast<CatalogImpl*>(api);
    BERRYDB_ASSUME_EQ(api, &impl->api_);
    return impl;
  }
  /** Computes the internal representation for a pointer from the public API. */
  static inline const CatalogImpl* FromApi(const Catalog* api) noexcept {
    const CatalogImpl* impl = reinterpret_cast<const CatalogImpl*>(api);
    BERRYDB_ASSUME_EQ(api, &impl->api_);
    return impl;
  }

  /** Computes the public API representation for this store. */
  inline constexpr Catalog* ToApi() noexcept { return &api_; }

  // See the public API documention for details.
  void Release();
  std::tuple<Status, CatalogImpl*> OpenCatalog(span<const uint8_t> name);
  std::tuple<Status, SpaceImpl*> OpenSpace(span<const uint8_t> name);

 private:
  /** Use CatalogImpl::Create() to obtain SpaceImpl instances. */
  CatalogImpl();
  /** Use Release() to destroy StoreImpl instances. */
  ~CatalogImpl();

  /* The public API version of this class. */
  Catalog api_;  // Must be the first class member.
};

}  // namespace berrydb

#endif  // BERRYDB_CATALOG_IMPL_H_
