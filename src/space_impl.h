// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_SPACE_IMPL_H_
#define BERRYDB_SPACE_IMPL_H_

#include "berrydb/space.h"
#include "./util/checks.h"

namespace berrydb {

/** Internal representation for the Space class in the public API. */
class SpaceImpl {
 public:
  static SpaceImpl* Create();

  SpaceImpl(const SpaceImpl&) = delete;
  SpaceImpl(SpaceImpl&&) = delete;
  SpaceImpl& operator=(const SpaceImpl&) = delete;
  SpaceImpl& operator=(SpaceImpl&&) = delete;

  /** Computes the internal representation for a pointer from the public API. */
  static inline SpaceImpl* FromApi(Space* api) noexcept {
    SpaceImpl* const impl = reinterpret_cast<SpaceImpl*>(api);
    BERRYDB_ASSUME_EQ(api, &impl->api_);
    return impl;
  }
  /** Computes the internal representation for a pointer from the public API. */
  static inline const SpaceImpl* FromApi(const Space* api) noexcept {
    const SpaceImpl* const impl = reinterpret_cast<const SpaceImpl*>(api);
    BERRYDB_ASSUME_EQ(api, &impl->api_);
    return impl;
  }

  /** Computes the public API representation for this store. */
  inline constexpr Space* ToApi() noexcept { return &api_; }

  // See the public API documention for details.
  void Release();

 private:
  /** Use SpaceImpl::Create() to obtain SpaceImpl instances. */
  SpaceImpl();
  /** Use Release() to destroy StoreImpl instances. */
  ~SpaceImpl();

  /* The public API version of this class. */
  Space api_;  // Must be the first class member.
};

}  // namespace berrydb

#endif  // BERRYDB_SPACE_IMPL_H_
