// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "berrydb/vfs.h"

#include <cstring>
#include <random>
#include <string>

#include "gtest/gtest.h"

#include "berrydb/status.h"
#include "../test/file_deleter.h"

namespace berrydb {

class VfsTest : public ::testing::Test {
 protected:
  VfsTest() : vfs_(DefaultVfs()), file_deleter_(kFileName) { }

  const std::string kFileName = "test_vfs.berry";
  constexpr static size_t kBlockShift = 12;
  Vfs* vfs_;
  FileDeleter file_deleter_;
  std::mt19937 rnd_;
};

TEST_F(VfsTest, OpenForBlockAccessOptions) {
  Status status;
  BlockAccessFile* file;
  size_t file_size;

  // Setup guarantees that the file does not exist.
  std::tie(status, file, file_size) = vfs_->OpenForBlockAccess(
      kFileName, kBlockShift, false, false);
  ASSERT_NE(Status::kSuccess, status);
  ASSERT_EQ(nullptr, file);

  std::tie(status, file, file_size) = vfs_->OpenForBlockAccess(
      kFileName, kBlockShift, true, true);
  ASSERT_EQ(Status::kSuccess, status);
  ASSERT_NE(nullptr, file);
  EXPECT_EQ(0U, file_size);
  EXPECT_EQ(Status::kSuccess, file->Close());

  // The ASSERT above guarantees that the file was created.
  std::tie(status, file, file_size) = vfs_->OpenForBlockAccess(
      kFileName, kBlockShift, true, true);
  ASSERT_NE(Status::kSuccess, status);
  EXPECT_EQ(nullptr, file);

  std::tie(status, file, file_size) = vfs_->OpenForBlockAccess(
      kFileName, kBlockShift, true, false);
  ASSERT_EQ(Status::kSuccess, status);
  ASSERT_NE(nullptr, file);
  EXPECT_EQ(0U, file_size);
  EXPECT_EQ(Status::kSuccess, file->Close());

  std::tie(status, file, file_size) = vfs_->OpenForBlockAccess(
      kFileName, kBlockShift, false, false);
  ASSERT_EQ(Status::kSuccess, status);
  ASSERT_NE(nullptr, file);
  EXPECT_EQ(0U, file_size);
  EXPECT_EQ(Status::kSuccess, file->Close());
}

TEST_F(VfsTest, BlockAccessFilePersistence) {
  uint8_t buffer[1 << kBlockShift], in_buffer[1 << kBlockShift];

  for (size_t i = 0; i < 1 << kBlockShift; ++i)
    buffer[i] = static_cast<uint8_t>(rnd_());

  Status status;
  BlockAccessFile* file;
  size_t file_size;
  std::tie(status, file, file_size) =
        vfs_->OpenForBlockAccess(kFileName, kBlockShift, true, false);
  ASSERT_EQ(Status::kSuccess, status);
  ASSERT_NE(nullptr, file);
  EXPECT_EQ(0U, file_size);
  EXPECT_EQ(Status::kSuccess, file->Write(buffer, 0));
  EXPECT_EQ(Status::kSuccess, file->Close());

  std::tie(status, file, file_size) =
      vfs_->OpenForBlockAccess(kFileName, kBlockShift, false, false);
  ASSERT_EQ(Status::kSuccess, status);
  ASSERT_NE(nullptr, file);
  EXPECT_EQ(1U << kBlockShift, file_size);
  EXPECT_EQ(Status::kSuccess, file->Read(0, in_buffer));
  EXPECT_EQ(Status::kSuccess, file->Close());

  EXPECT_EQ(0, std::memcmp(buffer, in_buffer, 1 << kBlockShift));
  EXPECT_EQ(Status::kSuccess, vfs_->RemoveFile(kFileName));
}

TEST_F(VfsTest, BlockAccessFileReadWriteOffsets) {
  uint8_t buffer[4][1 << kBlockShift], in_buffer[1 << kBlockShift];

  for (size_t i = 0; i < 4; ++i) {
    for (size_t j = 0; j < 1 << kBlockShift; ++j)
      buffer[i][j] = static_cast<uint8_t>(rnd_());
  }

  Status status;
  BlockAccessFile* file;
  size_t file_size;
  std::tie(status, file, file_size) =
      vfs_->OpenForBlockAccess(kFileName, kBlockShift, true, false);
  ASSERT_EQ(Status::kSuccess, status);
  ASSERT_NE(nullptr, file);
  EXPECT_EQ(0U, file_size);

  // Fill up the file with blocks [2, 1, 3, 0].
  EXPECT_EQ(Status::kSuccess, file->Write(buffer[2], 0 << kBlockShift));
  EXPECT_EQ(Status::kSuccess, file->Write(buffer[1], 1 << kBlockShift));
  EXPECT_EQ(Status::kSuccess, file->Write(buffer[3], 2 << kBlockShift));
  EXPECT_EQ(Status::kSuccess, file->Write(buffer[0], 3 << kBlockShift));

  // Read the blocks back out of order.
  EXPECT_EQ(Status::kSuccess, file->Read(2 << kBlockShift, in_buffer));
  EXPECT_EQ(0, std::memcmp(buffer[3], in_buffer, 1 << kBlockShift));

  EXPECT_EQ(Status::kSuccess, file->Read(1 << kBlockShift, in_buffer));
  EXPECT_EQ(0, std::memcmp(buffer[1], in_buffer, 1 << kBlockShift));

  EXPECT_EQ(Status::kSuccess, file->Read(0 << kBlockShift, in_buffer));
  EXPECT_EQ(0, std::memcmp(buffer[2], in_buffer, 1 << kBlockShift));

  EXPECT_EQ(Status::kSuccess, file->Read(3 << kBlockShift, in_buffer));
  EXPECT_EQ(0, std::memcmp(buffer[0], in_buffer, 1 << kBlockShift));

  // Rewrite blocks 0, 2, and 3.
  EXPECT_EQ(Status::kSuccess, file->Write(buffer[2], 2 << kBlockShift));
  EXPECT_EQ(Status::kSuccess, file->Write(buffer[0], 0 << kBlockShift));
  EXPECT_EQ(Status::kSuccess, file->Write(buffer[3], 3 << kBlockShift));

  // Read the blocks back out of order.
  EXPECT_EQ(Status::kSuccess, file->Read(1 << kBlockShift, in_buffer));
  EXPECT_EQ(0, std::memcmp(buffer[1], in_buffer, 1 << kBlockShift));

  EXPECT_EQ(Status::kSuccess, file->Read(0 << kBlockShift, in_buffer));
  EXPECT_EQ(0, std::memcmp(buffer[0], in_buffer, 1 << kBlockShift));

  EXPECT_EQ(Status::kSuccess, file->Read(3 << kBlockShift, in_buffer));
  EXPECT_EQ(0, std::memcmp(buffer[3], in_buffer, 1 << kBlockShift));

  EXPECT_EQ(Status::kSuccess, file->Read(2 << kBlockShift, in_buffer));
  EXPECT_EQ(0, std::memcmp(buffer[2], in_buffer, 1 << kBlockShift));

  EXPECT_EQ(Status::kSuccess, file->Close());
  EXPECT_EQ(Status::kSuccess, vfs_->RemoveFile(kFileName));
}

TEST_F(VfsTest, OpenForRandomAccessOptions) {
  Status status;
  RandomAccessFile* file;
  size_t file_size;

  // Setup guarantees that the file does not exist.
  std::tie(status, file, file_size) =
        vfs_->OpenForRandomAccess(kFileName, false, false);
  ASSERT_NE(Status::kSuccess, status);
  EXPECT_EQ(nullptr, file);

  std::tie(status, file, file_size) =
        vfs_->OpenForRandomAccess(kFileName, true, true);
  ASSERT_EQ(Status::kSuccess, status);
  ASSERT_NE(nullptr, file);
  EXPECT_EQ(0U, file_size);
  EXPECT_EQ(Status::kSuccess, file->Close());

  // The ASSERT above guarantees that the file was created.
  std::tie(status, file, file_size) =
        vfs_->OpenForRandomAccess(kFileName, true, true);
  ASSERT_NE(Status::kSuccess, status);
  EXPECT_EQ(nullptr, file);

  std::tie(status, file, file_size) =
      vfs_->OpenForRandomAccess(kFileName, true, false);
  ASSERT_EQ(Status::kSuccess, status);
  ASSERT_NE(nullptr, file);
  EXPECT_EQ(0U, file_size);
  EXPECT_EQ(Status::kSuccess, file->Close());

  std::tie(status, file, file_size) =
      vfs_->OpenForRandomAccess(kFileName, false, false);
  ASSERT_EQ(Status::kSuccess, status);
  ASSERT_NE(nullptr, file);
  EXPECT_EQ(0U, file_size);
  EXPECT_EQ(Status::kSuccess, file->Close());
}

TEST_F(VfsTest, RandomAccessFilePersistence) {
  uint8_t buffer_bytes[9000], in_buffer_bytes[9000];
  span<const uint8_t> buffer(buffer_bytes);
  span<uint8_t> in_buffer(in_buffer_bytes);

  for (size_t i = 0; i < sizeof(buffer); ++i)
    buffer_bytes[i] = static_cast<uint8_t>(rnd_());

  Status status;
  RandomAccessFile* file;
  size_t file_size;
  std::tie(status, file, file_size) = vfs_->OpenForRandomAccess(kFileName, true,
                                                                false);
  ASSERT_EQ(Status::kSuccess, status);
  ASSERT_NE(nullptr, file);
  EXPECT_EQ(0U, file_size);
  EXPECT_EQ(Status::kSuccess, file->Write(buffer.subspan(   0, 2000),    0));
  EXPECT_EQ(Status::kSuccess, file->Write(buffer.subspan(2000, 1000), 2000));
  EXPECT_EQ(Status::kSuccess, file->Write(buffer.subspan(3000, 3000), 3000));
  EXPECT_EQ(Status::kSuccess, file->Write(buffer.subspan(6000,  500), 6000));
  EXPECT_EQ(Status::kSuccess, file->Write(buffer.subspan(6500, 2500), 6500));
  EXPECT_EQ(Status::kSuccess, file->Close());

  std::tie(status, file, file_size) = vfs_->OpenForRandomAccess(
        kFileName, false, false);
  ASSERT_EQ(Status::kSuccess, status);
  ASSERT_NE(nullptr, file);
  EXPECT_EQ(9000U, file_size);
  EXPECT_EQ(Status::kSuccess, file->Read(   0, in_buffer.subspan(   0, 2500)));
  EXPECT_EQ(Status::kSuccess, file->Read(2500, in_buffer.subspan(2500,  500)));
  EXPECT_EQ(Status::kSuccess, file->Read(3000, in_buffer.subspan(3000, 3000)));
  EXPECT_EQ(Status::kSuccess, file->Read(6000, in_buffer.subspan(6000, 1000)));
  EXPECT_EQ(Status::kSuccess, file->Read(7000, in_buffer.subspan(7000, 2000)));
  EXPECT_EQ(Status::kSuccess, file->Close());

  EXPECT_EQ(buffer, in_buffer);
  EXPECT_EQ(Status::kSuccess, vfs_->RemoveFile(kFileName));
}

TEST_F(VfsTest, RandomAccessFileReadWriteOffsets) {
  uint8_t buffer_bytes[9000], in_buffer_bytes[9000];
  span<const uint8_t> buffer(buffer_bytes);
  span<uint8_t> in_buffer(in_buffer_bytes);

  for (size_t i = 0; i < sizeof(buffer); ++i)
    buffer_bytes[i] = static_cast<uint8_t>(rnd_());

  Status status;
  RandomAccessFile* file;
  size_t file_size;
  std::tie(status, file, file_size) = vfs_->OpenForRandomAccess(kFileName, true,
                                                                false);
  ASSERT_EQ(Status::kSuccess, status);
  ASSERT_NE(nullptr, file);
  EXPECT_EQ(0U, file_size);

  // Write the data in order.
  EXPECT_EQ(Status::kSuccess, file->Write(buffer.subspan(   0, 2000),    0));
  EXPECT_EQ(Status::kSuccess, file->Write(buffer.subspan(2000, 1000), 2000));
  EXPECT_EQ(Status::kSuccess, file->Write(buffer.subspan(3000, 3000), 3000));
  EXPECT_EQ(Status::kSuccess, file->Write(buffer.subspan(6000,  500), 6000));
  EXPECT_EQ(Status::kSuccess, file->Write(buffer.subspan(6500, 2500), 6500));

  // Read the data out of order.
  EXPECT_EQ(Status::kSuccess, file->Read(3000, in_buffer.subspan(3000, 3000)));
  EXPECT_EQ(Status::kSuccess, file->Read(7000, in_buffer.subspan(7000, 2000)));
  EXPECT_EQ(Status::kSuccess, file->Read(   0, in_buffer.subspan(   0, 2500)));
  EXPECT_EQ(Status::kSuccess, file->Read(6000, in_buffer.subspan(6000, 1000)));
  EXPECT_EQ(Status::kSuccess, file->Read(2500, in_buffer.subspan(2500,  500)));

  EXPECT_EQ(buffer, in_buffer);

  // Reset the data.
  for (size_t i = 0; i < sizeof(buffer); ++i)
    buffer_bytes[i] = static_cast<uint8_t>(rnd_());

  // Write the new data out of order.
  EXPECT_EQ(Status::kSuccess, file->Write(buffer.subspan(3000, 3000), 3000));
  EXPECT_EQ(Status::kSuccess, file->Write(buffer.subspan(6000,  500), 6000));
  EXPECT_EQ(Status::kSuccess, file->Write(buffer.subspan(   0, 2000),    0));
  EXPECT_EQ(Status::kSuccess, file->Write(buffer.subspan(6500, 2500), 6500));
  EXPECT_EQ(Status::kSuccess, file->Write(buffer.subspan(2000, 1000), 2000));

  // Read the new data out of order.
  EXPECT_EQ(Status::kSuccess, file->Read(6000, in_buffer.subspan(6000, 1000)));
  EXPECT_EQ(Status::kSuccess, file->Read(3000, in_buffer.subspan(3000, 3000)));
  EXPECT_EQ(Status::kSuccess, file->Read(   0, in_buffer.subspan(   0, 2500)));
  EXPECT_EQ(Status::kSuccess, file->Read(7000, in_buffer.subspan(7000, 2000)));
  EXPECT_EQ(Status::kSuccess, file->Read(2500, in_buffer.subspan(2500,  500)));

  EXPECT_EQ(buffer, in_buffer);

  EXPECT_EQ(Status::kSuccess, file->Close());
  EXPECT_EQ(Status::kSuccess, vfs_->RemoveFile(kFileName));
}

TEST_F(VfsTest, RemoveFile) {
  Status status;
  RandomAccessFile* file;
  size_t file_size;
  std::tie(status, file, file_size) = vfs_->OpenForRandomAccess(kFileName, true,
                                                                true);
  ASSERT_EQ(Status::kSuccess, status);
  ASSERT_NE(nullptr, file);
  EXPECT_EQ(0U, file_size);
  EXPECT_EQ(Status::kSuccess, file->Close());

  ASSERT_EQ(Status::kSuccess, vfs_->RemoveFile(kFileName));

  std::tie(status, file, file_size) = vfs_->OpenForRandomAccess(kFileName,
                                                                false, false);
  ASSERT_NE(Status::kSuccess, status);
}

}  // namespace berrydb
