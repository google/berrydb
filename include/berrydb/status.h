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

  // A large database was opened on a computer with a 32-bit CPU.
  //
  // This error can only occur when a database used on a 64-bit CPU is opened on
  // a 32-bit CPU.
  kDatabaseTooLarge = 8,

  // Valid values are in [kSuccess, kFirstInvalidValue).
  kFirstInvalidValue,  // This must remain at the end of the enum's block.
};

const char* StatusToCString(Status status) noexcept;

}  // namespace berrydb

#endif  // BERRYDB_INCLUDE_STATUS_H_
