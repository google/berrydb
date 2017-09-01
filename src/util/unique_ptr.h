// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_UTIL_UNIQUE_PTR_H_
#define BERRYDB_UTIL_UNIQUE_PTR_H_

#include <memory>

#include "./platform_deleter.h"
#include "berrydb/platform.h"

namespace berrydb {

/** std::unique_ptr variant that calls Release() to delete a pointer.
 *
 * std::unique_ptr (and, by extension, UniquePtr) is extremely cheap, but it is
 * not completely free, compared to a raw pointer that is known to be non-null.
 * Both assignment and destruction must check the wrapped pointer is null before
 * calling the delete operator. The checks contribute to code size and BTB
 * pollution.
 *
 * For these reasons, UniquePtr should not be considered a blanket replacement
 * for raw pointers, even when the ownership model matches. UniquePtr should be
 * used:
 *
 * 1) In tests, where the above concerns are irrelevant.
 * 2) In code that would contain the null checks anyways.
 * 3) In code where the pointer is destroyed rarely (e.g. Stores, not
 *    Transactions) and never / very rarely assigned to.
 */
template <typename T>
using UniquePtr = std::unique_ptr<T, PlatformDeleter<T>>;

}  // namespace berrydb

#endif  // BERRYDB_UTIL_UNIQUE_PTR_H_
