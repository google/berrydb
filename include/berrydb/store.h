// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_INCLUDE_STORE_H_
#define BERRYDB_INCLUDE_STORE_H_

#include "berrydb/platform.h"

namespace berrydb {

enum class Status : int;

class Transaction;

/** A key-value store. */
class Store {
 public:
  /** Starts a transaction against this store. */
  Transaction* CreateTransaction();

  /** Closes the store. */
  Status Close();
};

}  // namespace berrydb

#endif  // BERRYDB_INCLUDE_STORE_H_
