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
// 24: 8-byte number of pages in the store data file
// 32: 8-byte page index of the head of the free page list
// 40: 1-byte page shift (log2 of the page size)
// 41: 7-byte padding - reserved for future expansion, must be set to zero
//
// The format version number is a mechanism for future expansion. The number
// will remain at 0 until the format is stabilized. At that point, the version
// number will be bumped to 1, and files using format 0 will be rejected.

StoreHeader::StoreHeader(size_t page_shift, size_t page_count)
    : page_count(page_count)
#if DCHECK_IS_ON()
    , free_list_head_page(kInvalidFreeListHeadPage)
#endif  // DCHECK_IS_ON()
    , page_shift(page_shift)
    {
}

void StoreHeader::Serialize(uint8_t* to) {
  StoreUint64(StoreHeader::kGlobalMagic, to);
  StoreUint64(StoreHeader::kStoreMagic, to + 8);
  StoreUint64(0, to + 16);
  StoreUint64(page_count, to + 24);
  StoreUint64(free_list_head_page, to + 32);

  // This is guaranteed to set all the bytes 40..47 to 0.
  StoreUint64(0, to + 40);
  to[40] = page_shift;
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

  uint64_t number = LoadUint64(from + 24);
  page_count = number;
  if (page_count != number) {
    // This can happen on 32-bit systems that try to load a large database.
    return false;
  }

  number = LoadUint64(from + 32);
  free_list_head_page = number;
  if (free_list_head_page != number) {
    // This should not happen if the size test above passed. Still, we currently
    // consider that the extra code size of the check is preferrable to data
    // corruption or a difficult-to-debug crash.
    return false;
  }
  if (free_list_head_page == kInvalidFreeListHeadPage) {
    // Another data corruption case that is best caught early on.
    return false;
  }

  page_shift = from[40];
  if (page_shift >= 32) {
    // This should only happen due to data corruption.
    // TODO(pwnall): Move the page_shift limit into a constant that is also
    //               enforced at store creation time.
    return false;
  }

  return true;
}

}  // namespace berrydb
