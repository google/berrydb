// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_INCLUDE_SPACE_H_
#define BERRYDB_INCLUDE_SPACE_H_

#include "berrydb/platform.h"

namespace berrydb {

enum class Status : int;
class Transaction;

/**
 * An independent key/value namespace.
 *
 * Each (key/value name)space can be used by at most one write transaction OR by
 * an arbitrary number of read transactions. Once a transaction touches a space,
 * it is considered to be using the space until it commits or aborts.
 *
 * Conceptually, both keys and values are byte sequences (C++ std::string), and
 * the keys are sorted. Higher layers may assign more senamtics. For example,
 * catalogs are built on top of spaces.
 */
class Space {
  /** Releases the memory associated with the space.
   *
   * This must not be called during a transaction operation, such
   * as Transaction::Get(), that received the space as an argument.
   * After the operation completes, the space can be released, even
   * if the transaction is still alive.
   */
  void Release();

 private:
  friend class SpaceImpl;

  /** Use Catalog and Space methods to create Space instances. */
  Space() = default;
  /** Use Release() to destroy Space instances. */
  ~Space() = default;
};

}  // namespace berrydb

#endif  // BERRYDB_INCLUDE_SPACE_H_
