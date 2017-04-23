// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_PLATFORM_HASHING_H_
#define BERRYDB_PLATFORM_HASHING_H_

// Embedders who implement their own hashing can replace the functions below to
// reduce code size and/or increase performance.

#include <functional>

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
template<typename T>
struct PointerHasher {
  inline size_t operator()(T* pointer) const noexcept {
    std::hash<T*> hasher;
    return hasher(pointer);
  }
};

// Hash specialization used for pairs of pointers and size_t.
template<typename T>
struct PointerSizeHasher {
  inline size_t operator()(std::pair<T*, size_t> pair) const noexcept {
    SizeHasher size_hasher;
    PointerHasher<T> pointer_hasher;

    size_t h1 = size_hasher(pair.first);
    size_t h2 = pointer_hasher(pair.second);

    static_assert(sizeof(size_t) == 4 || sizeof(size_t) == 8,
        "This implementation assumes that hash outputs are 32-bit or 64-bit");

    if (sizeof(size_t) == 4) {  // The compiler should optimize away the if.
      h2 *= 0xcc9e2d51;
      h2 = (h2 << 15) | (h2 >> 17);
      return h1 ^ h2;

    } else {
      h2 *= 0xc6a4a7935bd1e995;
      h2 = (h2 << 47) | (h2 >> 17);
      return h1 ^ h2;
    }
  }
};

}  // namespace berrydb

#endif  // BERRYDB_PLATFORM_HASHING_H_
