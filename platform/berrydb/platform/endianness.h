// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_PLATFORM_ENDIANNESS_H_
#define BERRYDB_PLATFORM_ENDIANNESS_H_

// Embedders can reimplement the methods below to emit and consume databases
// across platforms with different endianness.

#include "./dcheck.h"
#include "./types.h"

namespace berrydb {

/** Reads a 64-bit unsigned integer from an aligned buffer.
 *
 * Consumers should assume that the integer is stored in a cross-platform
 * manner, but not depend on that in testing. This lets the embedder choose
 * portability or extra speed on big-endian platforms.
 *
 * @param  from memory holding the integer; must be 8-byte-aligned
 * @return      the integer stored at the given location
 */
inline uint64_t LoadUint64(const uint8_t* from) noexcept {
  DCHECK_EQ(reinterpret_cast<uintptr_t>(from) & 7, 0);
  return *(reinterpret_cast<const uint64_t*>(from));
}

/** Stores a 64-bit unsigned integer to an aligned buffer.
 *
 * Consumers should assume that the integer is stored in a cross-platform
 * manner, but not depend on that in testing. This lets the embedder choose
 * portability or extra speed on big-endian platforms.
 *
 * @param  from memory holding the integer; must be 8-byte-aligned
 * @return      the integer stored at the given location
 */
inline void StoreUint64(uint64_t value, uint8_t* to) noexcept {
  DCHECK_EQ(reinterpret_cast<uintptr_t>(to) & 7, 0);
  *(reinterpret_cast<uint64_t*>(to)) = value;
}

}  // namespace berrydb

#endif  // BERRYDB_PLATFORM_ENDIANNESS_H_
