// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_UTIL_PLATFORM_ALLOCATOR_H_
#define BERRYDB_UTIL_PLATFORM_ALLOCATOR_H_

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "berrydb/platform.h"

namespace berrydb {

/** std::allocator variant that proxies to the memory allocator in platform.h.
 *
 * Implemented for use with the standard library's containers.
 *
 * This can (and must) be used with any standard library type that takes an
 * Allocator type argument. PlatformAllocator has no state, so it does not add a
 * memory tax to types that were designed to take advantage of the empty base
 * optimization.
 *
 * Example:
 *    DO NOT use: std::vector<Page>
 *    DO use:     std::vector<Page, PlatformAllocator<Page>>
 */
template<typename T>
struct PlatformAllocator {
  typedef T value_type;
  typedef std::true_type is_empty;

  // The types below are deprecated in C++17, but are still needed by the
  // compilers that we need to support right now.
  typedef T* pointer;
  typedef const T* const_pointer;
  typedef T& reference;
  typedef const T& const_reference;
  template <typename U> struct rebind { typedef PlatformAllocator<U> other; };

  inline T* allocate(std::size_t count) {
    return reinterpret_cast<T*>(Allocate(sizeof(T) * count));
  }
  inline void deallocate(T* data, std::size_t count) {
    Deallocate(reinterpret_cast<void*>(data), sizeof(T) * count);
  }

  inline PlatformAllocator() noexcept = default;
  inline PlatformAllocator(const PlatformAllocator& other) noexcept = default;
  template<typename U>
  inline PlatformAllocator(const PlatformAllocator<U>& other) noexcept {
    UNUSED(other);
  };
};

template<typename T, typename U> inline constexpr bool operator==(
    const PlatformAllocator<T>&, const PlatformAllocator<U>&) noexcept {
  return true;
}
template<typename T, typename U> inline constexpr bool operator!=(
    const PlatformAllocator<T>&, const PlatformAllocator<U>&) noexcept {
  return false;
}

}  // namespace berrydb

#endif  // BERRYDB_UTIL_PLATFORM_ALLOCATOR_H_
