// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "./store_header.h"

#include "../util/checks.h"
#include "../util/endianness.h"

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
#if BERRYDB_CHECK_IS_ON()
    , free_list_head_page(kInvalidFreeListHeadPage)
#endif  // BERRYDB_CHECK_IS_ON()
    , page_shift(page_shift)
    {
  BERRYDB_ASSUME_GT(page_shift, 0U);
}

void StoreHeader::Serialize(span<uint8_t> to) {
  StoreUint64(StoreHeader::kGlobalMagic, to.subspan(0, 8));
  StoreUint64(StoreHeader::kStoreMagic, to.subspan(8, 8));
  StoreUint64(0, to.subspan(16, 8));
  StoreUint64(page_count, to.subspan(24, 8));
  StoreUint64(free_list_head_page, to.subspan(32, 8));

  // This is guaranteed to set all the bytes 40..47 to 0.
  StoreUint64(0, to.subspan(40, 8));
  BERRYDB_ASSUME_LT(page_shift, 32U);
  to[40] = static_cast<uint8_t>(page_shift);
}

bool StoreHeader::Deserialize(span<const uint8_t> from) {
  const uint64_t global_magic = LoadUint64(from.subspan(0, 8));
  if (UNLIKELY(global_magic != kGlobalMagic))
    return false;

  const uint64_t store_magic = LoadUint64(from.subspan(8, 8));
  if (UNLIKELY(store_magic != kStoreMagic))
    return false;

  const uint64_t format_version = LoadUint64(from.subspan(16, 8));
  if (UNLIKELY(format_version != 0))
    return false;

  const uint64_t page_count64 = LoadUint64(from.subspan(24, 8));
  page_count = static_cast<size_t>(page_count64);
  if (UNLIKELY(page_count != page_count64)) {
    // This can happen on 32-bit systems that try to load a large database.
    return false;
  }

  const uint64_t free_list_head_page64 = LoadUint64(from.subspan(32, 8));
  free_list_head_page = static_cast<size_t>(free_list_head_page64);
  if (UNLIKELY(free_list_head_page != free_list_head_page64)) {
    // This should not happen if the size test above passed. Still, we currently
    // consider that the extra code size of the check is preferrable to data
    // corruption or a difficult-to-debug crash.
    return false;
  }
  if (UNLIKELY(free_list_head_page == kInvalidFreeListHeadPage)) {
    // Another data corruption case that is best caught early on.
    return false;
  }

  page_shift = from[40];
  if (UNLIKELY(page_shift >= 32)) {
    // This should only happen due to data corruption.
    // TODO(pwnall): Move the page_shift limit into a constant that is also
    //               enforced at store creation time.
    return false;
  }

  return true;
}

}  // namespace berrydb
