// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cmath>
#include <random>

#include "benchmark/benchmark.h"

#include "berrydb/platform.h"
#include "berrydb/status.h"
#include "berrydb/vfs.h"
#include "../test/file_deleter.h"
#include "../util/unique_ptr.h"

namespace berrydb {

class VfsBenchmark : public benchmark::Fixture {
 public:
  VfsBenchmark() : vfs_(DefaultVfs()), deleter_(kFileName) {}

  void SetUp(const benchmark::State& state) override {
    block_size_ = state.range(0);
    block_shift_ = static_cast<size_t>(std::log2(block_size_));
    DCHECK_EQ(block_size_, 1U << block_shift_);

    block_data_ = reinterpret_cast<uint8_t*>(Allocate(block_size_));
    DCHECK(block_data_ != nullptr);
  }

  void TearDown(const benchmark::State& state) override {
    Deallocate(block_data_, block_size_);
    UNUSED(state);
  }

 protected:
  const std::string kFileName = "bench_vfs.file";

  Vfs* vfs_;
  // Must precede UniquePtr members, because on Windows all file handles must be
  // closed before the files can be deleted.
  FileDeleter deleter_;

  std::mt19937 rnd_;
  size_t block_size_;
  size_t block_shift_;
  uint8_t* block_data_;
};


BENCHMARK_DEFINE_F(VfsBenchmark, RandomBlockWrites)(benchmark::State& state) {
  UniquePtr<BlockAccessFile> file;
  BlockAccessFile* raw_file;
  size_t raw_file_size;
  Status status = vfs_->OpenForBlockAccess(
      deleter_.path(), block_shift_, true, false, &raw_file, &raw_file_size);
  if (status != Status::kSuccess) {
    state.SkipWithError("Vfs::OpenForBlockAccess failed.");
    return;
  }
  file.reset(raw_file);

  size_t block_count = state.range(1);
  for (size_t i = 0; i < block_count; ++i) {
    status = file->Write(block_data_, i << block_shift_, block_size_);
    if (status != Status::kSuccess) {
      state.SkipWithError("BlockAccessFile::Write failed. (initial fill)");
      return;
    }
  }
  if (file->Sync() != Status::kSuccess) {
    state.SkipWithError("BlockAccessFile::Sync failed.");
    return;
  }

  while (state.KeepRunning()) {
    size_t block_number = rnd_() % block_count;

    if (file->Write(block_data_, block_number << block_shift_, block_size_) !=
        Status::kSuccess) {
      state.SkipWithError("BlockAccessFile::Write failed. (random block)");
      return;
    }
    if (file->Sync() != Status::kSuccess) {
      state.SkipWithError("BlockAccessFile::Sync failed.");
      return;
    }
  }

  state.SetBytesProcessed(state.iterations() << block_shift_);
  state.SetItemsProcessed(state.iterations());
}

BENCHMARK_REGISTER_F(
    VfsBenchmark, RandomBlockWrites)->RangeMultiplier(2)->Ranges(
    {{4096, 65536},  // Block size.
    {1024, 1024}});  // Total number of blocks in the file.

BENCHMARK_DEFINE_F(VfsBenchmark, LogWrites)(benchmark::State& state) {
  UniquePtr<RandomAccessFile> file;
  RandomAccessFile* raw_file;
  size_t raw_file_size;
  Status status = vfs_->OpenForRandomAccess(
      deleter_.path(), true, false, &raw_file, &raw_file_size);
  if (status != Status::kSuccess) {
    state.SkipWithError("Vfs::OpenForBlockAccess failed.");
    return;
  }
  file.reset(raw_file);

  for (size_t block_number = 0; state.KeepRunning(); ++block_number) {
    if (file->Write(block_data_, block_number << block_shift_, block_size_) !=
        Status::kSuccess) {
      state.SkipWithError("RandomAccessFile::Write failed.");
      return;
    }
    if (file->Sync() != Status::kSuccess) {
      state.SkipWithError("RandomAccessFile::Sync failed.");
      return;
    }
  }
  
  state.SetBytesProcessed(state.iterations() << block_shift_);
  state.SetItemsProcessed(state.iterations());
}

BENCHMARK_REGISTER_F(VfsBenchmark, LogWrites)->RangeMultiplier(2)->Range(
    4096, 65536);  // Log record size.

}  // namespace berrydb
