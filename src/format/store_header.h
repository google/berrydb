// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_STORE_HEADER_H_
#define BERRYDB_STORE_HEADER_H_

#include "berrydb/platform.h"

namespace berrydb {

/** The data in a store file's header.
 *
 * The in-memory header data layout is optimized for computation. The methods
 * Serialize() and Deserialize() translate between the in-memory layout and the
 * on-disk layout.
 */
struct StoreHeader {
  /** Used when reading header data from a file.
   *
   * All the struct members have unspecified default values, so this constructor
   * should not generate any code. */
  inline StoreHeader() noexcept = default;

  /** Used by the StoreImpl constructor. */
  StoreHeader(size_t page_shift, size_t page_count);

  /** Stores the header data into a buffer using the on-disk layout.
   *
   * @param  to the buffer that receives the on-disk layout header data
   */
  void Serialize(uint8_t* to);

  /** Reads the header data from a buffer that uses the on-disk layout.
   *
   * The method replaces this instance's state. If the read succeeds, the
   * instance will reflect data in the header. If the read fails, the instance's
   * state is undefined.
   *
   * @param  from the buffer that stores the on-disk layout header data; the
   *              data should come from a previous Serialize() call
   * @return      true if the read succeeded
   */
  bool Deserialize(const uint8_t* from);

  /** The number of pages in the store's data file.
   *
   * This number is closely related to the size of the store's data file, and
   * includes the pages on the free list, which are not currently storing
   * meaningful data. */
  size_t page_count;

  /** 0-based index of the page at the head of the free list. */
  size_t free_list_head_page;

  /** Base-2 log of the store's page size.
   *
   * The store's page size can be computed as 1 << page_shift, or
   * 2 ** page_shift. */
  size_t page_shift;

  /** The size of a serialized store header, in bytes.
   *
   * This is a constant because the store header only has fixed-width fields. */
  static constexpr size_t kSerializedSize = 48;

  /** Magic number used to tag all BerryDB files.
   *
   * The number is encoded as "BerryDB " on little-endian systems. */
  static constexpr uint64_t kGlobalMagic = 0x4265727279444220;

  /** Magic number used to tag BerryDB store files.
   *
   * The number is encoded as "DBStore " on little-endian systems. */
  static constexpr uint64_t kStoreMagic = 0x444253746f726520;

  /** Invalid value for the free_list_head_page header field.
   *
   * The first page in a data file stores the header, so it cannot be used to
   * store free list information.
   */
  static constexpr size_t kInvalidFreeListHeadPage = 0;
};

}  // namespace berrydb

#endif  // BERRYDB_STORE_HEADER_H_
