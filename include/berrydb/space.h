// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_INCLUDE_SPACE_H_
#define BERRYDB_INCLUDE_SPACE_H_

#include "berrydb/platform.h"

namespace berrydb {

class Transaction;

/**
 * An independent namespace for keys in a store.
 *
 * Each space can be used by at most one write transaction OR by an arbitrary
 * number of read transactions.
 */
class Space;

}  // namespace berrydb

#endif  // BERRYDB_INCLUDE_SPACE_H_
