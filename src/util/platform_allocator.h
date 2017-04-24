// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_UTIL_PLATFORM_ALLOCATOR_H_
#define BERRYDB_UTIL_PLATFORM_ALLOCATOR_H_

#include "berrydb/platform.h"

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace berrydb {

/** std::allocator variant that uses BerryDB's platform.
 *
 * Implemented for use with the standard library's containers. */
template<typename T>
struct PlatformAllocator {
  typedef T value_type;
  typedef std::true_type is_empty;

  // The following are deprecated in C++17, but are still needed to build the
  // code right now.
  typedef T* pointer;
  typedef const T* const_pointer;
  typedef T& reference;
  typedef const T& const_reference;
  template <typename U> struct rebind { typedef PlatformAllocator<U> other; };

  inline T* allocate(std::size_t bytes) {
    return reinterpret_cast<T*>(Allocate(bytes));
  }
  inline void deallocate(T* data, std::size_t bytes) {
    Deallocate(reinterpret_cast<void*>(data), bytes);
  }

  inline PlatformAllocator() noexcept = default;
  inline PlatformAllocator(const PlatformAllocator& other) noexcept = default;
  template<typename U>
  inline PlatformAllocator(const PlatformAllocator<U>& other) noexcept {
    UNUSED(other);
  };
};

static_assert(std::is_empty<PlatformAllocator<int>>::value,
    "PlatformAllocator should not have state");

}  // namespace berrydb

#endif  // BERRYDB_UTIL_PLATFORM_ALLOCATOR_H_
