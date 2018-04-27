// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_PLATFORM_ENDIANNESS_H_
#define BERRYDB_PLATFORM_ENDIANNESS_H_

// Embedders can reimplement the methods below to emit and consume databases
// across platforms with different endianness.

#include <cassert>

#include "berrydb/span.h"
#include "berrydb/types.h"

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
template <typename ElementType>
inline constexpr uint64_t LoadUint64(span<ElementType> from) noexcept {
  // DCHECK doesn't work in constexpr.
  assert((reinterpret_cast<uintptr_t>(from.data()) & 7) == 0);
  assert(from.size_bytes() == 8);

  return *(reinterpret_cast<const uint64_t*>(from.data()));
}

/** Stores a 64-bit unsigned integer to an aligned buffer.
 *
 * Consumers should assume that the integer is stored in a cross-platform
 * manner, but not depend on that in testing. This lets the embedder choose
 * portability or extra speed on big-endian platforms.
 *
 * @param  value the value to be stored
 * @param  to    memory that will receive the integer; must be 8-byte-aligned
 * @return       the integer stored at the given location
 */
template <
    typename ElementType,
    typename = std::enable_if_t<!std::is_const<ElementType>::value>>
inline void StoreUint64(uint64_t value, span<ElementType> to) noexcept {
  // DCHECK doesn't work in constexpr.
  assert((reinterpret_cast<uintptr_t>(to.data()) & 7) == 0);
  assert(to.size_bytes() == 8);

  *(reinterpret_cast<uint64_t*>(to.data())) = value;
}

}  // namespace berrydb

#endif  // BERRYDB_PLATFORM_ENDIANNESS_H_
