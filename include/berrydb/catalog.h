// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_INCLUDE_CATALOG_H_
#define BERRYDB_INCLUDE_CATALOG_H_

#include "berrydb/string_view.h"

namespace berrydb {

class Space;
enum class Status : int;

/** A directory of other catalogs and key/value namespaces.
 *
 * A catalog is a key/value namespace, where the keys are byte sequences (C++
 * std::strings) and the values are other catalogs or spaces, which hold user
 * data.
 *
 * Catalogs are accessed and modified outside the transaction system. Each
 * operation on a catalog implicitly executes in its own transaction. The user
 * is responsible for avoiding data races.
 */
class Catalog {
  /** Opens a catalog listed in this catalog.
   *
   * No other change operation (Create* / Delete*) may be issued concurrently
   * on this catalog.
   *
   * @param name   the name of the catalog to be opened
   * @param result if the operation succeeds, receives a pointer to the opened
   *               catalog
   * @return       kNotFound if the catalog does not contain an entry with the
   *               given name; otherwise, most likely kSuccess or kIoError
   */
  Status OpenCatalog(string_view name, Catalog** result);

  /** Opens a (key/value name)space listed in this catalog.
   *
   * No other change operation (Create* / Delete*) may be issued concurrently
   * on this catalog.
   *
   * @param name   the name of the catalog to be opened
   * @param result if the operation succeeds, receives a pointer to the opened
   *               space
   * @return       kNotFound if the catalog does not contain an entry with the
   *               given name; otherwise, most likely kSuccess or kIoError
   */
  Status OpenSpace(string_view name, Space** result);

  /** Releases the memory associated with the catalog.
   *
   * This must not be called during an operation, such as
   * Transaction::CreateSpace(), that received the catalog as an argument.
   * After the operation completes, the catalog can be released, even
   * if the transaction is still alive.
   */
  void Release();

 private:
  friend class CatalogImpl;

  /**
   * Create instances via Store::RootCatalog() and Catalog/Transaction methods.
   */
  constexpr Catalog() noexcept = default;

  /** Use Release() to destroy Catalog instances. */
  ~Catalog() noexcept = default;
};

}  // namespace berrydb

#endif  // BERRYDB_INCLUDE_CATALOG_H_
