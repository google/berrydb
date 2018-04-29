// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "./space_impl.h"

#include "berrydb/platform.h"

namespace berrydb {

static_assert(std::is_standard_layout<SpaceImpl>::value,
    "SpaceImpl must be a standard layout type so its public API can be "
    "exposed cheaply");

SpaceImpl* SpaceImpl::Create() {
  void* heap_block = Allocate(sizeof(SpaceImpl));
  SpaceImpl* space = new (heap_block) SpaceImpl();
  DCHECK_EQ(heap_block, static_cast<void*>(space));
  return space;
}

SpaceImpl::SpaceImpl() = default;

SpaceImpl::~SpaceImpl() = default;

void SpaceImpl::Release() {
  this->~SpaceImpl();
  void* heap_block = static_cast<void*>(this);
  Deallocate(heap_block, sizeof(SpaceImpl));
}

}  // namespace berrydb
