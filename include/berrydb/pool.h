// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

namespace berrydb {

class Store;

/** A pool of resources that can be shared among stores. */
class Pool {
 public:
  Store* Open(std::string path);

  /** Construct a new pool. */
  static Pool* New();
};

}  // namespace berrydb
