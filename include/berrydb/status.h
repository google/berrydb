// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_INCLUDE_STATUS_H_
#define BERRYDB_INCLUDE_STATUS_H_

namespace berrydb {

enum class Status : int {
  // Everything went well.
  kSuccess = 0,

  // The desired key or file was not found.
  kNotFound = 1,

  // The unerlying filesystem returned an error.
  kIoError = 2,

  // The resource pool is over-utilized.
  kPoolFull = 3,
};

}  // namespace berrydb

#endif  // BERRYDB_INCLUDE_TRANSACTION_H_
