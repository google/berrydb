// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_PLATFORM_HASHING_H_
#define BERRYDB_PLATFORM_HASHING_H_

// Embedders who implement their own hashing can replace the functions below to
// reduce code size and/or increase performance.

#include <functional>
#include <utility>

#include "./types.h"

namespace berrydb {

// Hash specialization used for size_t integers.
struct SizeHasher {
  inline size_t operator()(size_t number) const noexcept {
    std::hash<size_t> hasher;
    return hasher(number);
  }
};

// Hash specialization used for pointers.
template<typename T> struct PointerHasher {
  inline size_t operator()(T* pointer) const noexcept {
    std::hash<T*> hasher;
    return hasher(pointer);
  }
};

// Combines the value of two hashes.
template<typename T> struct HashCombiner {
  inline T operator()(T h1, T h2) const noexcept {
    static_assert(
        std::is_same<T, uint32_t>::value ||
        std::is_same<T, uint64_t>::value,
        "This implementation assumes size_t is uint32_t or uint64_t");

    h2 *= Multiplier();
    h2 = (h2 << (sizeof(T) * 8 - 17)) | (h2 >> 17);
    return h1 ^ h2;
  }
 private:
  static constexpr T Multiplier();
  static constexpr T RotateFactor() { return 17; }
};
template<> constexpr uint64_t HashCombiner<uint64_t>::Multiplier() {
  return 0xc6a4a7935bd1e995;
}
template<> constexpr uint32_t HashCombiner<uint32_t>::Multiplier() {
  return 0xcc9e2d51;
}

// Hash specialization used for pairs of pointers and size_t.
template<typename T> struct PointerSizeHasher {
  inline size_t operator()(const std::pair<T*, size_t> pair) const noexcept {
    size_t h1 = PointerHasher<T>()(pair.first);
    size_t h2 = SizeHasher()(pair.second);
    return HashCombiner<size_t>()(h1, h2);
  }
};

}  // namespace berrydb

#endif  // BERRYDB_PLATFORM_HASHING_H_
