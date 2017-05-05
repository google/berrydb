// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "berrydb/vfs.h"

#include <cstring>
#include <random>
#include <string>

#include "gtest/gtest.h"

#include "berrydb/status.h"

namespace berrydb {

class VfsTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    vfs_ = DefaultVfs();
    vfs_->DeleteFile(kFileName);
  }

  virtual void TearDown() {
    vfs_->DeleteFile(kFileName);
  }

  const std::string kFileName = "test_vfs.berry";
  Vfs* vfs_;
  std::mt19937 rnd_;
};

#if defined(_WIN32) || defined(WIN32)
TEST_F(VfsTest, DISABLED_OpenForBlockAccessOptions) {
#else  // defined(_WIN32) || defined(WIN32)
TEST_F(VfsTest, OpenForBlockAccessOptions) {
#endif  // defined(_WIN32) || defined(WIN32)
  BlockAccessFile* file = nullptr;

  // Setup guarantees that the file does not exist.
  ASSERT_NE(
      Status::kSuccess,
      vfs_->OpenForBlockAccess(kFileName, 12, false, false, &file));
  EXPECT_EQ(nullptr, file);

  ASSERT_EQ(
      Status::kSuccess,
      vfs_->OpenForBlockAccess(kFileName, 12, true, true, &file));
  file->Close();

  // The ASSERT above guarantees that the file was created.
  file = nullptr;
  ASSERT_NE(
      Status::kSuccess,
      vfs_->OpenForBlockAccess(kFileName, 12, true, true, &file));
  EXPECT_EQ(nullptr, file);

  ASSERT_EQ(
      Status::kSuccess,
      vfs_->OpenForBlockAccess(kFileName, 12, true, false, &file));
  file->Close();

  ASSERT_EQ(
      Status::kSuccess,
      vfs_->OpenForBlockAccess(kFileName, 12, false, false, &file));
  file->Close();
}

TEST_F(VfsTest, BlockAccessFilePersistence) {
  uint8_t buffer[1 << 12], read_buffer[1 << 12];
  BlockAccessFile* file;

  for (size_t i = 0; i < 1 << 12; ++i)
    buffer[i] = static_cast<uint8_t>(rnd_());

  ASSERT_EQ(Status::kSuccess, vfs_->OpenForBlockAccess(
      kFileName, 12, true, false, &file));
  EXPECT_EQ(Status::kSuccess, file->Write(buffer, 0, 1 << 12));
  EXPECT_EQ(Status::kSuccess, file->Close());

  ASSERT_EQ(Status::kSuccess, vfs_->OpenForBlockAccess(
      kFileName, 12, false, false, &file));
  EXPECT_EQ(Status::kSuccess, file->Read(0, 1 << 12, read_buffer));
  EXPECT_EQ(Status::kSuccess, file->Close());

  EXPECT_EQ(0, std::memcmp(buffer, read_buffer, 1 << 12));
  EXPECT_EQ(Status::kSuccess, vfs_->DeleteFile(kFileName));
}

TEST_F(VfsTest, BlockAccessFileReadWriteOffsets) {
  uint8_t buffer[4][1 << 12], read_buffer[1 << 12];
  BlockAccessFile* file;

  for (size_t i = 0; i < 4; ++i) {
    for (size_t j = 0; j < 1 << 12; ++j)
      buffer[i][j] = static_cast<uint8_t>(rnd_());
  }

  ASSERT_EQ(Status::kSuccess, vfs_->OpenForBlockAccess(
      kFileName, 12, true, false, &file));

  // Fill up the file with blocks [2, 1, 3, 0].
  EXPECT_EQ(Status::kSuccess, file->Write(buffer[2], 0 << 12, 1 << 12));
  EXPECT_EQ(Status::kSuccess, file->Write(buffer[1], 1 << 12, 1 << 12));
  EXPECT_EQ(Status::kSuccess, file->Write(buffer[3], 2 << 12, 1 << 12));
  EXPECT_EQ(Status::kSuccess, file->Write(buffer[0], 3 << 12, 1 << 12));

  // Read the blocks back out of order.
  EXPECT_EQ(Status::kSuccess, file->Read(2 << 12, 1 << 12, read_buffer));
  EXPECT_EQ(0, std::memcmp(buffer[3], read_buffer, 1 << 12));

  EXPECT_EQ(Status::kSuccess, file->Read(1 << 12, 1 << 12, read_buffer));
  EXPECT_EQ(0, std::memcmp(buffer[1], read_buffer, 1 << 12));

  EXPECT_EQ(Status::kSuccess, file->Read(0 << 12, 1 << 12, read_buffer));
  EXPECT_EQ(0, std::memcmp(buffer[2], read_buffer, 1 << 12));

  EXPECT_EQ(Status::kSuccess, file->Read(3 << 12, 1 << 12, read_buffer));
  EXPECT_EQ(0, std::memcmp(buffer[0], read_buffer, 1 << 12));

  // Rewrite blocks 0, 2, and 3.
  EXPECT_EQ(Status::kSuccess, file->Write(buffer[2], 2 << 12, 1 << 12));
  EXPECT_EQ(Status::kSuccess, file->Write(buffer[0], 0 << 12, 1 << 12));
  EXPECT_EQ(Status::kSuccess, file->Write(buffer[3], 3 << 12, 1 << 12));

  // Read the blocks back out of order.
  EXPECT_EQ(Status::kSuccess, file->Read(1 << 12, 1 << 12, read_buffer));
  EXPECT_EQ(0, std::memcmp(buffer[1], read_buffer, 1 << 12));

  EXPECT_EQ(Status::kSuccess, file->Read(0 << 12, 1 << 12, read_buffer));
  EXPECT_EQ(0, std::memcmp(buffer[0], read_buffer, 1 << 12));

  EXPECT_EQ(Status::kSuccess, file->Read(3 << 12, 1 << 12, read_buffer));
  EXPECT_EQ(0, std::memcmp(buffer[3], read_buffer, 1 << 12));

  EXPECT_EQ(Status::kSuccess, file->Read(2 << 12, 1 << 12, read_buffer));
  EXPECT_EQ(0, std::memcmp(buffer[2], read_buffer, 1 << 12));

  EXPECT_EQ(Status::kSuccess, file->Close());
  EXPECT_EQ(Status::kSuccess, vfs_->DeleteFile(kFileName));
}

}  // namespace berrydb
