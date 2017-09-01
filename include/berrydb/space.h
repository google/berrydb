// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_INCLUDE_BERRYDB_SPACE_H_
#define BERRYDB_INCLUDE_BERRYDB_SPACE_H_

namespace berrydb {

enum class Status : int;
class Transaction;

/**
 * A (key-value name)space, or a mapping (function) from strings to strings.
 *
 * "Space" stands for key-value namespace, and is a mapping (function) from
 * strings to strings. In other words, a space can have at most one value
 * associated with a given key.
 *
 * For the purposes here, "string" is used in the C++ sense of std::string, so a
 * string is actually a sequence of bytes, not a sequence of (most likely
 * Unicode) codepoints or characters. Keys can be accessed efficiently in stored
 * order.
 *
 * Higher layers may assign more senamtics to both keys and values. For example,
 * catalogs are internally built on top of spaces.
 *
 * Each (key/value name)space can be used by at most one write transaction OR by
 * an arbitrary number of read transactions. Once a transaction touches a space,
 * it is considered to be using the space until it commits or it is rolled back.
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
  constexpr Space() noexcept = default;
  /** Use Release() to destroy Space instances. */
  ~Space() noexcept = default;
};

}  // namespace berrydb

#endif  // BERRYDB_INCLUDE_BERRYDB_SPACE_H_
