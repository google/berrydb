// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "./store_header.h"

#include "berrydb/platform.h"

namespace berrydb {

// The store header format is as follows:
//
//  0: 8-byte global magic number - "BerryDB "
//  8: 8-byte store magic number - "DBStore "
// 16: 8-byte format version number, might be broken up in the future - 0
// 24: 8-byte page index of the head of the free page list
// 32: 1-byte page shift (log2 of the page size)
// 33: 7-byte padding - reserved for future expansion, must be set to zero
//
// The format version number is a mechanism for future expansion. The number
// will remain at 0 until the format is stabilized. At that point, the version
// number will be bumped to 1, and files using format 0 will be rejected.

void StoreHeader::Serialize(uint8_t* to) {
  StoreUint64(StoreHeader::kGlobalMagic, to);
  StoreUint64(StoreHeader::kStoreMagic, to + 8);
  StoreUint64(0, to + 16);
  StoreUint64(free_list_head_page, to + 24);

  // This is guaranteed to set all the bytes 32..39 to 0.
  StoreUint64(0, to + 32);
  to[32] = page_shift;
}

bool StoreHeader::Deserialize(const uint8_t *from) {
  uint64_t global_magic = LoadUint64(from);
  if (global_magic != kGlobalMagic)
    return false;

  uint64_t store_magic = LoadUint64(from + 8);
  if (store_magic != kStoreMagic)
    return false;

  uint64_t format_version = LoadUint64(from + 16);
  if (format_version != 0)
    return false;

  uint64_t page_number = LoadUint64(from + 24);
  free_list_head_page = page_number;
  if (free_list_head_page != page_number) {
    // This can happen on 32-bit systems that try to load a large database.
    return false;
  }

  page_shift = from[32];
  if (page_shift >= 32)
    return false;

  return true;
}

}  // namespace berrydb
