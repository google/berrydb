// Copyright 2018 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_UTIL_SPAN_UTIL_H_
#define BERRYDB_UTIL_SPAN_UTIL_H_

#include <algorithm>
#include <type_traits>

#include "berrydb/span.h"
#include "./checks.h"

namespace berrydb {

// TODO(pwnall): Switch to std::is_const_v and std::is_convertible_v in C++17.

/** std::fill convenience wrapper for spans.
 *
 * @param data  the span whose content is filled; must not have a const type
 * @param value used to fill the span
 */
template <
    typename ElementType,
    typename ValueType,
    typename = std::enable_if_t<!std::is_const<ElementType>::value>,
    typename = std::enable_if_t<std::is_convertible<ValueType,
                                                    ElementType>::value>>
constexpr void FillSpan(span<ElementType> data, ValueType value) {
  // TODO(pwnall): Benchmark replacing std::fill with std::memset for bytes.
  std::fill(data.begin(), data.end(), static_cast<ElementType>(value));
}

/** std::copy convenience wrapper for spans.
 *
 * The source span's type must match the destination span type, modulo const.
 * The source span may be const, while the destination span must not be const.
 * The destination span must be at least as large as the source span.
 *
 * @param from the span whose data is copied
 * @param to   the span that the data is copied to
 */
template <
    typename FromType,
    typename ToType,
    typename = std::enable_if_t<!std::is_const<ToType>::value>,
    typename = std::enable_if_t<std::is_same<std::remove_cv_t<FromType>,
                                             ToType>::value>>
constexpr void CopySpan(span<FromType> from, span<ToType> to) {
  // BERRYDB_CHECK_LE does not work in constexpr.
  BERRYDB_CHECK(from.size() <= to.size());

  // TODO(pwnall): Benchmark replacing std::copy with std::memcpy.
  std::copy(from.begin(), from.end(), to.begin());
}

}  // namespace berrydb

#endif  // BERRYDB_UTIL_SPAN_UTIL_H_
