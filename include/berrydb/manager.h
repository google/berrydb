// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

namespace berrydb {

class Database;

class Manager {
 public:
  Database* Open(std::string path);
};

}  // namespace berrydb
