// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "./file_deleter.h"

#include "berrydb/vfs.h"

namespace berrydb {

FileDeleter::FileDeleter(const std::string& path) : path_(path) {
  DefaultVfs()->RemoveFile(path_);
}

FileDeleter::~FileDeleter() {
  DefaultVfs()->RemoveFile(path_);
}

}  // namespace berrydb
