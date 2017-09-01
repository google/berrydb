// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_PLATFORM_HASHING_H_
#define BERRYDB_PLATFORM_HASHING_H_

// Embedders who implement their own hashing can replace the functions below to
// reduce code size and/or increase performance.

#include <functional>
#include <utility>

#include "berrydb/types.h"

namespace berrydb {

// Hash specialization used for size_t integers.
struct SizeHasher {
  inline size_t operator()(size_t number) const noexcept {
    std::hash<size_t> hasher;
    return hasher(number);
  }
};

// Hash specialization used for pointers.
template <typename PointedType>
struct PointerHasher {
  inline size_t operator()(PointedType* pointer) const noexcept {
    std::hash<PointedType*> hasher;
    return hasher(pointer);
  }
};

// Hash specialization used for pairs of pointers and size_t.
template <typename T>
struct PointerSizeHasher {
  inline size_t operator()(const std::pair<T*, size_t> pair) const noexcept {
    size_t h1 = PointerHasher<T>()(pair.first);
    size_t h2 = SizeHasher()(pair.second);
    return HashCombiner<size_t>()(h1, h2);
  }

 private:
  // Combines the value of two hashes.
  template <typename SizeType, size_t SizeOfSizeType = sizeof(SizeType)>
  struct HashCombiner {
    inline SizeType operator()(SizeType h1, SizeType h2) const
        noexcept = delete;
  };
  template <typename SizeType>
  struct HashCombiner<SizeType, 8> {
    inline SizeType operator()(SizeType h1, SizeType h2) const noexcept {
      h2 *= static_cast<SizeType>(0xc6a4a7935bd1e995);
      h2 = (h2 << 47) | (h2 >> 17);
      return h1 ^ h2;
    }
  };
  template <typename SizeType>
  struct HashCombiner<SizeType, 4> {
    inline SizeType operator()(SizeType h1, SizeType h2) const noexcept {
      h2 *= static_cast<SizeType>(0xcc9e2d51);
      h2 = (h2 << 15) | (h2 >> 17);
      return h1 ^ h2;
    }
  };
};

}  // namespace berrydb

#endif  // BERRYDB_PLATFORM_HASHING_H_
