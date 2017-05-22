// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_CATALOG_IMPL_H_
#define BERRYDB_CATALOG_IMPL_H_

#include "berrydb/catalog.h"

namespace berrydb {

class SpaceImpl;

/** Internal representation for the Catalog class in the public API. */
class CatalogImpl {
 public:
  static CatalogImpl* Create();

  /** Computes the internal representation for a pointer from the public API. */
  static inline CatalogImpl* FromApi(Catalog* api) noexcept {
    CatalogImpl* impl = reinterpret_cast<CatalogImpl*>(api);
    DCHECK_EQ(api, &impl->api_);
    return impl;
  }
  /** Computes the internal representation for a pointer from the public API. */
  static inline const CatalogImpl* FromApi(const Catalog* api) noexcept {
    const CatalogImpl* impl = reinterpret_cast<const CatalogImpl*>(api);
    DCHECK_EQ(api, &impl->api_);
    return impl;
  }

  /** Computes the public API representation for this store. */
  inline Catalog* ToApi() noexcept { return &api_; }

  // See the public API documention for details.
  void Release();
  Status OpenCatalog(string_view name, CatalogImpl** result);
  Status OpenSpace(string_view name, SpaceImpl** result);

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
