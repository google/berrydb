// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_STORE_HEADER_H_
#define BERRYDB_STORE_HEADER_H_

#include "berrydb/platform.h"

namespace berrydb {

struct StoreHeader {
  bool Deserialize(const uint8_t* from);

  /** Stores the data in this structure into a buffer meant for on-disk storage.
   *
   * @param  to [description]
   * @return    [description]
   */
  void Serialize(uint8_t* to);

  size_t page_shift;
  size_t free_list_head_page;


  /** The size of a store header, in bytes.
   *
   * This is a constant because the store header only has fixed-width fields. */
  static constexpr size_t kSizeInBytes = 40;

  /** Magic number used to tag all BerryDB files.
   *
   * The number is encoded as "BerryDB " on little-endian systems. */
  static constexpr uint64_t kGlobalMagic = 0x4265727279444220;

  /** Magic number used to tag BerryDB store files.
   *
   * The number is encoded as "DBStore " on little-endian systems. */
  static constexpr uint64_t kStoreMagic = 0x444253746f726520;
};

}  // namespace berrydb

#endif  // BERRYDB_STORE_HEADER_H_
