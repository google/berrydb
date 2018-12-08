// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_UTIL_ENDIANNESS_H_
#define BERRYDB_UTIL_ENDIANNESS_H_

#include "berrydb/platform.h"
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
  // DCHECK_EQ doesn't work in constexpr.
  DCHECK((reinterpret_cast<uintptr_t>(from.data()) & 7) == 0);
  DCHECK(from.size_bytes() == 8);

  return PlatformLoadUint64(from.data());
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
  DCHECK_EQ((reinterpret_cast<uintptr_t>(to.data()) & 7), 0U);
  DCHECK_EQ(to.size_bytes(), 8U);

  return PlatformStoreUint64(value, to.data());
}

}  // namespace berrydb

#endif  // BERRYDB_UTIL_ENDIANNESS_H_
