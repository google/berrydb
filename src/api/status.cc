// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "berrydb/status.h"

#include "../util/checks.h"

namespace berrydb {

const char* StatusToCString(Status status) noexcept {
  switch (status) {
  case Status::kSuccess:
    return "Success";
  case Status::kIoError:
    return "I/O Error";
  case Status::kNotFound:
    return "Not Found";
  case Status::kAlreadyLocked:
    return "Already Locked";
  case Status::kAlreadyExists:
    return "Already Exists";
  case Status::kAlreadyClosed:
    return "Already Closed";
  case Status::kPoolFull:
    return "Page Pool Full";
  case Status::kDataCorrupted:
    return "Data Corrupted";
  case Status::kDatabaseTooLarge:
    return "Database Too Large";
  case Status::kFirstInvalidValue:
    // Needed to avoid a (very useful otherwise) compiler warning.
    break;
  }

  BERRYDB_UNREACHABLE();
}

}  // namespace berrydb
