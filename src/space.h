// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_SPACE_H_
#define BERRYDB_SPACE_H_

#include "berrydb/space.h"

#include "berrydb/platform.h"

namespace berrydb {

class Transaction;

// Full definition for the class declaration in include/berrydb/space.h.
// The class specification is there.
class Space {
 public:
  Space(size_t root_page) : root_page_(root_page) { };

 private:
  /** Page number that holds the root of the space's data structure. */
  size_t root_page_;
};

}  // namespace berrydb

#endif  // BERRYDB_INCLUDE_SPACE_H_
