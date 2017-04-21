// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_INCLUDE_STORE_H_
#define BERRYDB_INCLUDE_STORE_H_

namespace berrydb {

class Transaction;

/** A key-value store. */
class Store {
 public:
  Transaction* NewTransaction();
};

}  // namespace berrydb

#endif  // BERRYDB_INCLUDE_STORE_H_
