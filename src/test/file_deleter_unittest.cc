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
  RandomAccessFile* file;
  size_t file_size;
  ASSERT_EQ(Status::kSuccess, vfs->OpenForRandomAccess(
      kFileName, true, false, &file, &file_size));
  file->Close();

  {
    FileDeleter deleter(kFileName);
    // The file should have been deleted when FileDeleter was constructed.
    EXPECT_NE(Status::kSuccess, vfs->OpenForRandomAccess(
        kFileName, false, false, &file, &file_size));

    EXPECT_EQ(kFileName, deleter.path());

    ASSERT_EQ(Status::kSuccess, vfs->OpenForRandomAccess(
        kFileName, true, false, &file, &file_size));
    file->Close();
  }
  // The file should have been deleted when FileDeleter was destroyed.
  EXPECT_NE(Status::kSuccess, vfs->OpenForRandomAccess(
      kFileName, false, false, &file, &file_size));
}

}  // namespace berrydb
