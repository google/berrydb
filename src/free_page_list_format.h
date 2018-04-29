// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_FREE_PAGE_LIST_FORMAT_H
#define BERRYDB_FREE_PAGE_LIST_FORMAT_H

#include "berrydb/platform.h"
#include "berrydb/span.h"

namespace berrydb {

/** Implementation details for FreePageList.
 *
 * This class should only be used by the FreePageList implementation and tests.
 */
class FreePageListFormat {
 public:
  /** Reads the offset of the next entry (page ID) to be added to a list page.
   *
   * @param  page_data the data buffer of a page used for free page list data
   * @return           0-based byte offset in the page data where the next
   *                   entry (page ID of a free page) will be stored; if the
   *                   data is not corrupted, this is a number in [0, page size]
   */
  static inline constexpr size_t NextEntryOffset(
      span<const uint8_t> page_data) noexcept {
    // The span size is only used in asserts.
    //
    // The implementation relies on the compiler to optimize away the span's
    // size and only pass the pointer in release builds.
    //
    // Casting is safe because the page size must be smaller than size_t. The
    // next entry offset must be at most page_size, so it must be smaller than
    // size_t.
    //
    // The 64-bit load is safe because page data is at least 64-bit-aligned, and
    // list data pages are made up of 64-bit numbers.
    return static_cast<size_t>(
        LoadUint64(page_data.subspan(kNextEntryOffset, 8)));
  }

  /** Sets the offset of the next entry (page ID) to be added to a list page.
   *
   * @param  next_entry_offset the 0-based byte offset in the page data where
   *                           the next entry (page ID of a free page) will be
   *                           stored; should be a number in [0, page size]
   * @param  page_data         the writable data buffer of a page used for free
   *                           page list data
   */
  static inline void SetNextEntryOffset(size_t next_entry_offset,
                                        span<uint8_t> page_data) noexcept {
    // The span size is only used in asserts.
    //
    // The implementation relies on the compiler to optimize away the span's
    // size and only pass the pointer in release builds.
    //
    // The next_entry_offset value is not DCHECKed on purpose, so this method
    // can be used by data corruption tests.
    //
    // The 64-bit store is safe because page data is at least 64-bit-aligned,
    // and list data pages are made up of 64-bit numbers.
    StoreUint64(static_cast<uint64_t>(next_entry_offset),
                page_data.subspan(kNextEntryOffset, 8));
  }

  /** Reads the page ID of the successor to a list page.
   *
   * The return value is a 64-bit number (uint64_t), so the caller has to deal
   * with the possibility that a 32-bit computer is used to open a large
   * database created by a 64-bit computer, whose page IDs might exceed 32 bits.
   * Depending on the situation, the caller can either give up (and return
   * Status::kDatabaseTooLarge), or can pass through 64-bit page IDs.
   *
   * @param  page_data the data buffer of a page used for free page list data
   * @return           the page ID of the next data page in the list; if the
   *                   data is not corrupted, this will not be kInvalidPageId
   */
  static constexpr inline uint64_t NextPageId64(
      span<const uint8_t> page_data) noexcept {
    // The span size is only used in asserts.
    //
    // The implementation relies on the compiler to optimize away the span's
    // size and only pass the pointer in release builds.
    //
    // The 64-bit load is safe because page data is at least 64-bit-aligned, and
    // list data pages are made up of 64-bit numbers.
    return LoadUint64(page_data.subspan(kNextPageIdOffset, 8));
  }

  /** Sets the page ID of the successor to a list page.
   *
   * @param  page_data      the writable data buffer of a page used for free
   *                        page list data
   * @return next_page_id64 the page ID of the next data page in the list;
   *                        should not be kInvalidPageId
   */
  static inline void SetNextPageId64(uint64_t next_page_id64,
                                     span<uint8_t> page_data) noexcept {
    // The span size is only used in the DCHECK.
    //
    // The implementation relies on the compiler to optimize away the span's
    // size and only pass the pointer in release builds.
    //
    // The next_page_id64 value is not DCHECKed on purpose, so this method can
    // be used by data corruption tests.
    //
    // The 64-bit store is safe because page data is at least 64-bit-aligned,
    // and list data pages are made up of 64-bit numbers.
    return StoreUint64(next_page_id64, page_data.subspan(kNextPageIdOffset, 8));
  }

  /** True if a list entry (page ID) offset is guranteed to be invalid.
   *
   * This method should be used before using an offset that comes from an
   * external source (disk) to read a list entry from a page buffer.
   *
   * @param  entry_offset the list entry offset to be validated
   * @param  page_size    the page size of the store data file where the free
   *                      page list resides
   * @return              true if the given offset is guaranteed to be invalid
   */
  static inline constexpr bool IsCorruptEntryOffset(size_t entry_offset,
                                                    size_t page_size) noexcept {
    static_assert(
        (kEntrySize & (kEntrySize - 1)) == 0,
        "kEntrySize must be a power of two for bit masking tricks to work");

    // The checks below are the minimum needed to protect against an invalid
    // memory access.
    //
    // A more complete check would also cover the case where entry_offset points
    // to list metadata (below kFirstEntryOffset). However, the extra check
    // costs more code, and doesn't have significant benefits. If the free list
    // pages get corrupted, the page pointers are most likely incorrect, and
    // that will cause the system to completely trash the store data file. The
    // best we can do in this case is not crash.
    return entry_offset >= page_size || (entry_offset & (kEntrySize - 1)) != 0;
  }

  /** The offset of the first free entry in a list page.
   *
   * All list operations need to look at this number. It is at the beginning of
   * the page so accessing it requires slightly less code than accessing any
   * other field. */
  static constexpr size_t kNextEntryOffset = 0;
  /** The offset of the next list page's id in a list page. */
  static constexpr size_t kNextPageIdOffset = 8;

  /** The offset of the first entry (page id) in a list page. */
  static constexpr size_t kFirstEntryOffset = 16;
  /** The size of each entry (page id) in a free page list page.
   *
   * This is currently set to 8 bytes, needed by page IDs in large databases. */
  static constexpr size_t kEntrySize = 8;

  // TODO(pwnall): Consider / evaluate using base-128 varints instead of 8-byte
  //               ints for page entries. Instead of relying on kEntrySize,
  //               removing an entry would have to scan back for the first byte
  //               without the top bit set.
};

}  // namespace berrydb

#endif  // BERRYDB_FREE_PAGE_LIST_FORMAT_H
