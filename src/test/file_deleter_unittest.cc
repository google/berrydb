// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "./file_deleter.h"

#include "gtest/gtest.h"

#include "berrydb/status.h"
#include "berrydb/vfs.h"

namespace berrydb {

TEST(FileDeleterTest, DeletesFileBeforeAndAfter) {
  const std::string kFileName = "file_deleter_test.empty";

  Vfs* vfs = DefaultVfs();
  Status status;
  RandomAccessFile* file;
  size_t file_size;
  std::tie(status, file, file_size) = vfs->OpenForRandomAccess(
      kFileName, true, false);
  ASSERT_EQ(Status::kSuccess, status);
  file->Close();

  {
    FileDeleter deleter(kFileName);
    // The file should have been deleted when FileDeleter was constructed.
    std::tie(status, file, file_size) = vfs->OpenForRandomAccess(
        kFileName, false, false);
    EXPECT_NE(Status::kSuccess, status);

    EXPECT_EQ(kFileName, deleter.path());

    std::tie(status, file, file_size) = vfs->OpenForRandomAccess(
        kFileName, true, false);
    ASSERT_EQ(Status::kSuccess, status);
    file->Close();
  }
  // The file should have been deleted when FileDeleter was destroyed.
  std::tie(status, file, file_size) = vfs->OpenForRandomAccess(
      kFileName, false, false);
  EXPECT_NE(Status::kSuccess, status);
}

}  // namespace berrydb
