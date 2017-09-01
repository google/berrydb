// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_INCLUDE_BERRYDB_STATUS_OSTREAM_H_
#define BERRYDB_INCLUDE_BERRYDB_STATUS_OSTREAM_H_

// This header defines ostream operators for the project's API objects. These
// operators are intended to be used for debugging or diagnostic logging, and
// are not suitable for persisting state or for user interfaces.

#include <ostream>

namespace berrydb {

enum class Status : int;

std::ostream& operator <<(std::ostream& stream, Status status);

}  // namespace berrydb

#endif  // BERRYDB_INCLUDE_BERRYDB_STATUS_OSTREAM_H_
