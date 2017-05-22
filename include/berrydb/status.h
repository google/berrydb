// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_INCLUDE_STATUS_H_
#define BERRYDB_INCLUDE_STATUS_H_

namespace berrydb {

enum class Status : int {
  // Everything went well.
  kSuccess = 0,

  // The unerlying filesystem returned an error.
  kIoError = 1,

  // The desired key or file was not found.
  kNotFound = 2,

  // The resource has already been locked by another user.
  kAlreadyLocked = 3,

  // An object with the given key already exists.
  kAlreadyExists = 4,

  // Close() has already been called.
  kAlreadyClosed = 5,

  // The resource pool is over-utilized.
  kPoolFull = 6,

  // The underlying data was corrupted.
  kDataCorrupted = 7,
};

}  // namespace berrydb

#endif  // BERRYDB_INCLUDE_TRANSACTION_H_
