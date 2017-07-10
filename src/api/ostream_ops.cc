// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "berrydb/ostream_ops.h"

#include <ostream>

#include "berrydb/status.h"

namespace berrydb {

std::ostream& operator <<(std::ostream& stream, Status status) {
  return stream << "[Status: " << StatusToCString(status) << "]";
}

}  // namespace berrydb
