// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_UTIL_PLATFORM_DELETER_H_
#define BERRYDB_UTIL_PLATFORM_DELETER_H_

namespace berrydb {

/** std::default_delete variant that calls Release on the pointer.
 *
 * All the project's types use a custom memory allocator defined in platform.h.
 * To avoid bugs, their constructors and destructors are marked private.
 * operator new is replaced Create() methods, and operator delete is replaced by
 * Release() methods.
 *
 * This deleter invokes the Release() method, and must be used with smart
 * pointers targeting the project's types. The deleter is stateless, so it adds
 * no memory overhead to smart pointer implementations that take advantage of
 * the empty base optimization.
 *
 * Instead of using this deleter directly with std::unique_ptr, use the
 * UniquePtr alias defined in util/unique_ptr.h.
 */
template <typename T>
struct PlatformDeleter {
  inline PlatformDeleter() noexcept = default;

  template <typename U>
  inline PlatformDeleter(const PlatformDeleter<U>& other) noexcept {
    UNUSED(other);
  }

  void operator()(T* data) const { data->Release(); }
};

/** PlatformDeleter cannot be used with arrays. */
template <typename T>
struct PlatformDeleter<T[]> {
  void operator()(T* data) const = delete;
};

}  // namespace berrydb

#endif  // BERRYDB_UTIL_PLATFORM_DELETER_H_
