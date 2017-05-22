// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifdef _MSC_VER  // Attempt to silence security warnings in MSVC.
#define _CRT_SECURE_NO_WARNINGS
#endif  // _MSC_VER

#include "berrydb/vfs.h"

// TODO(pwnall): This is not as efficient as having separate POSIX and Windows
//               implementations. Write the separate implementations in the
//               nearby future.

#include <cstdio>

#include "berrydb/platform.h"
#include "berrydb/status.h"
#include "../util/platform_allocator.h"

namespace berrydb {

namespace {

std::FILE* OpenLibcFile(
    const std::string& file_path, bool create_if_missing,
    bool error_if_exists) {
  const char* cpath = file_path.c_str();

  if (error_if_exists) {
    // NOTE: This relies on C2011.
    return std::fopen(cpath, "wb+x");
  }

  if (create_if_missing) {
    // NOTE: It might be faster to freopen.
    FILE* fp = std::fopen(cpath, "ab+");
    if (fp != nullptr)
      std::fclose(fp);
  }

  return std::fopen(cpath, "rb+");
}

Status ReadLibcFile(
    std::FILE* fp, size_t offset, size_t byte_count, uint8_t* buffer) {
  // NOTE(pwnall): On POSIX, we'd want to use pread instead of fseek() and
  //               fread().

  if (std::fseek(fp, offset, SEEK_SET) != 0) {
    // ferror() can be checked if we want to return more detailed errors.
    return Status::kIoError;
  }

  if (std::fread(buffer, byte_count, 1, fp) != 1) {
    // feof() and ferror() have more details on the error.
    return Status::kIoError;
  }

  return Status::kSuccess;
}

Status WriteLibcFile(
    std::FILE* fp, const uint8_t* buffer, size_t offset, size_t byte_count) {
  // NOTE(pwnall): On POSIX, we'd want to use pwrite instead of fseek() and
  //               fwrite().

  if (std::fseek(fp, offset, SEEK_SET) != 0) {
    // ferror() can be checked if we want to return more detailed errors.
    return Status::kIoError;
  }

  if (std::fwrite(buffer, byte_count, 1, fp) != 1) {
    // feof() and ferror() have more details on the error.
    return Status::kIoError;
  }

  return Status::kSuccess;
}

Status SyncLibcFile(std::FILE* fp) {
  // HACK(pwnall): fflush() does not have the guarantees we require, but is
  //               the closest that the C/C++ standard has to offer.
  // This should use fdatasync() on POSIX and FlushFileBuffers() on Windows.
  return (std::fflush(fp) == 0) ? Status::kSuccess : Status::kIoError;
}

}  // anonymous namespace

class LibcBlockAccessFile : public BlockAccessFile {
 public:
  LibcBlockAccessFile(FILE* fp, size_t block_shift)
      : fp_(fp)
#if DCHECK_IS_ON()
      , block_size_(1 << block_shift)
#endif  // DCHECK_IS_ON()
      {
    DCHECK(fp != nullptr);

    UNUSED(block_shift);

    // Disable buffering, because we're doing block I/O.
    // NOTE(pwnall): This is an incomplete substitute for O_DIRECT.
    std::setbuf(fp, nullptr);
  }

  Status Read(size_t offset, size_t byte_count, uint8_t* buffer) override {
#if DCHECK_IS_ON()
    DCHECK_EQ(offset & (block_size_ - 1), 0);
    DCHECK_EQ(byte_count & (block_size_ - 1), 0);
#endif  // DCHECK_IS_ON()

    return ReadLibcFile(fp_, offset, byte_count, buffer);
  }

  Status Write(uint8_t* buffer, size_t offset, size_t byte_count) override {
#if DCHECK_IS_ON()
    DCHECK_EQ(offset & (block_size_ - 1), 0);
    DCHECK_EQ(byte_count & (block_size_ - 1), 0);
#endif  // DCHECK_IS_ON()

    return WriteLibcFile(fp_, buffer, offset, byte_count);
  }

  Status Sync() override { return SyncLibcFile(fp_); }

  Status Close() override {
    void* heap_block = reinterpret_cast<void*>(this);
    this->~LibcBlockAccessFile();
    Deallocate(heap_block, sizeof(LibcBlockAccessFile));
    return Status::kSuccess;
  }

 protected:
  ~LibcBlockAccessFile() {
    fclose(fp_);
  }

 private:
  std::FILE* fp_;

#if DCHECK_IS_ON()
  size_t block_size_;
#endif  // DCHECK_IS_ON()
};

class LibcRandomAccessFile : public RandomAccessFile {
 public:
  LibcRandomAccessFile(FILE* fp) : fp_(fp) {
    DCHECK(fp != nullptr);
  }

  Status Read(size_t offset, size_t byte_count, uint8_t* buffer) override {
    return ReadLibcFile(fp_, offset, byte_count, buffer);
  }

  Status Write(
      const uint8_t* buffer, size_t offset, size_t byte_count) override {
    return WriteLibcFile(fp_, buffer, offset, byte_count);
  }

  Status Flush() override {
    return (std::fflush(fp_) == 0) ? Status::kSuccess : Status::kIoError;
  }

  Status Sync() override { return SyncLibcFile(fp_); }

  Status Close() override {
    void* heap_block = reinterpret_cast<void*>(this);
    this->~LibcRandomAccessFile();
    Deallocate(heap_block, sizeof(LibcRandomAccessFile));
    return Status::kSuccess;
  }

 protected:
  ~LibcRandomAccessFile() {
    fclose(fp_);
  }

 private:
  std::FILE* fp_;
};

class LibcVfs : public Vfs {
 public:
  Status OpenForRandomAccess(
      const std::string& file_path, bool create_if_missing,
      bool error_if_exists, RandomAccessFile** result,
      size_t* result_size) override {
    FILE* fp = OpenLibcFile(file_path, create_if_missing, error_if_exists);
    if (fp == nullptr)
      return Status::kIoError;

    if (std::fseek(fp, 0, SEEK_END) != 0) {
      // ferror() can be checked if we want to return more detailed errors.
      return Status::kIoError;
    }
    size_t file_size = std::ftell(fp);

    void* heap_block = Allocate(sizeof(LibcRandomAccessFile));
    LibcRandomAccessFile* file = new (heap_block) LibcRandomAccessFile(fp);
    DCHECK_EQ(heap_block, reinterpret_cast<void*>(file));
    *result = file;
    *result_size = file_size;
    return Status::kSuccess;
  }
  Status OpenForBlockAccess(
      const std::string& file_path, size_t block_shift,
      bool create_if_missing, bool error_if_exists,
      BlockAccessFile** result) override {
    FILE* fp = OpenLibcFile(file_path, create_if_missing, error_if_exists);
    if (fp == nullptr)
      return Status::kIoError;

    void* heap_block = Allocate(sizeof(LibcBlockAccessFile));
    LibcBlockAccessFile* file = new (heap_block) LibcBlockAccessFile(
        fp, block_shift);
    DCHECK_EQ(heap_block, reinterpret_cast<void*>(file));
    *result = file;
    return Status::kSuccess;
  }

  Status DeleteFile(const std::string& file_path) override {
    if(std::remove(file_path.c_str()) != 0)
      return Status::kIoError;

    return Status::kSuccess;
  }
};

// TODO(pwnall): Put DefaultVfs() behind an #ifdef, so embedders can use the
//               built-in VFS, but still supply their own default.
Vfs* DefaultVfs() {
  // TODO(pwnall): Check whether this is threadsafe.
  static Vfs* vfs = new (Allocate(sizeof(Vfs))) LibcVfs();

  return vfs;
}

}  // namespace berrydb
