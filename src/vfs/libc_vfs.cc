// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "berrydb/vfs.h"

// TODO(pwnall): This is not as efficient as having separate POSIX and Windows
//               implementations. Write the separate implementations in the
//               nearby future.

#include <cstdio>

#include "berrydb/platform.h"
#include "../util/platform_allocator.h"

namespace berrydb {

using string =
    std::basic_string<char, std::char_traits<char>, PlatformAllocator<char>>;

class LibcBlockAccessFile : public BlockAccessFile {
 public:
  LibcBlockAccessFile(FILE* fp, size_t block_shift)
      : fp_(fp), block_shift_(block_shift), block_size_(1 << block_shift) {
    DCHECK(fp != nullptr);

    // Disable buffering, because we're doing block I/O.
    // NOTE(pwnall): This is an incomplete substitute for O_DIRECT.
    std::setbuf(fp, nullptr);
  }

  Status Read(size_t offset, size_t bytes, uint8_t* buffer) override {
    DCHECK_EQ(offset & (block_size_ - 1), 0);
    DCHECK_EQ(bytes & (block_size_ - 1), 0);

    // NOTE(pwnall): On POSIX, we'd want to use pread instead of fseek() and
    //               fread().

    if (std::fseek(fp_, offset, SEEK_SET) != 0) {
      // ferror() can be checked if we want to return more detailed errors.
      return Status::kIoError;
    }

    size_t block_count = bytes >> block_shift_;
    if (std::fread(buffer, block_size_, block_count, fp_) != block_count) {
      // feof() and ferror() have more details on the error.
      return Status::kIoError;
    }

    return Status::kSuccess;
  }

  Status Write(uint8_t* buffer, size_t offset, size_t bytes) override {
    DCHECK_EQ(offset & (block_size_ - 1), 0);
    DCHECK_EQ(bytes & (block_size_ - 1), 0);

    // NOTE(pwnall): On POSIX, we'd want to use pwrite instead of fseek() and
    //               fwrite().

    if (std::fseek(fp_, offset, SEEK_SET) != 0) {
      // ferror() can be checked if we want to return more detailed errors.
      return Status::kIoError;
    }

    size_t block_count = bytes >> block_shift_;
    if (std::fwrite(buffer, block_size_, block_count, fp_) != block_count) {
      // feof() and ferror() have more details on the error.
      return Status::kIoError;
    }

    return Status::kSuccess;
  }

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
  size_t block_shift_;
  size_t block_size_;
};

class LibcVfs : public Vfs {
 public:
  Status OpenForBlockAccess(
      const std::string& file_path, size_t block_shift,
      bool create_if_missing, bool error_if_exists,
      BlockAccessFile** result) override {
    const char* cpath = file_path.c_str();

    FILE* fp;
    if (error_if_exists) {
      // NOTE: This relies on C2011.
      fp = std::fopen(cpath, "wb+x");
    } else {
      if (create_if_missing) {
        // NOTE: It might be faster to freopen.
        fp = std::fopen(cpath, "ab+");
        if (fp)
          std::fclose(fp);
      }

      fp = std::fopen(cpath, "rb+");
    }

    if (!fp)
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
