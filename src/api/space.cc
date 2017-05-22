// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "berrydb/space.h"

#include "../space_impl.h"

namespace berrydb {

void Space::Release() {
  return SpaceImpl::FromApi(this)->Release();
}

}  // namespace berrydb
