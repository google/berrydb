// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cmath>
#include <random>
#include <tuple>

#include "benchmark/benchmark.h"

#include "berrydb/platform.h"
#include "berrydb/status.h"
#include "berrydb/vfs.h"
#include "../test/file_deleter.h"
#include "../util/checks.h"
#include "../util/unique_ptr.h"

namespace berrydb {

class VfsBenchmark : public benchmark::Fixture {
 public:
  VfsBenchmark() : vfs_(DefaultVfs()), deleter_(kFileName) {}

  void SetUp(const benchmark::State& state) override {
    block_size_ = static_cast<size_t>(state.range(0));
    block_shift_ = static_cast<size_t>(std::log2(block_size_));
    BERRYDB_ASSUME_EQ(block_size_, static_cast<size_t>(1) << block_shift_);

    block_bytes_ = reinterpret_cast<uint8_t*>(Allocate(block_size_));
    BERRYDB_ASSUME(block_bytes_ != nullptr);
  }

  void TearDown(MAYBE_UNUSED const benchmark::State& state) override {
    Deallocate(block_bytes_, block_size_);
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
  uint8_t* block_bytes_;
};


BENCHMARK_DEFINE_F(VfsBenchmark, RandomBlockWrites)(benchmark::State& state) {
  UniquePtr<BlockAccessFile> file;
  Status status;
  BlockAccessFile* raw_file;
  size_t raw_file_size;
  std::tie(status, raw_file, raw_file_size) = vfs_->OpenForBlockAccess(
      deleter_.path(), block_shift_, true, false);
  if (status != Status::kSuccess) {
    state.SkipWithError("Vfs::OpenForBlockAccess failed.");
    return;
  }
  file.reset(raw_file);

  size_t block_count = static_cast<size_t>(state.range(1));
  span<const uint8_t> block_data(block_bytes_, block_size_);
  for (size_t i = 0; i < block_count; ++i) {
    status = file->Write(block_data, i << block_shift_);
    if (status != Status::kSuccess) {
      state.SkipWithError("BlockAccessFile::Write failed. (initial fill)");
      return;
    }
  }
  if (file->Sync() != Status::kSuccess) {
    state.SkipWithError("BlockAccessFile::Sync failed.");
    return;
  }

  for (auto _ : state) {
    size_t block_number = rnd_() % block_count;

    if (file->Write(block_data, block_number << block_shift_) !=
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
  Status status;
  RandomAccessFile* raw_file;
  size_t raw_file_size;
  std::tie(status, raw_file, raw_file_size) = vfs_->OpenForRandomAccess(
      deleter_.path(), true, false);
  if (status != Status::kSuccess) {
    state.SkipWithError("Vfs::OpenForBlockAccess failed.");
    return;
  }
  file.reset(raw_file);

  size_t block_number = 0;
  span<const uint8_t> block_data(block_bytes_, block_size_);
  for (auto _ : state) {
    if (file->Write(block_data, block_number << block_shift_) !=
        Status::kSuccess) {
      state.SkipWithError("RandomAccessFile::Write failed.");
      return;
    }
    if (file->Sync() != Status::kSuccess) {
      state.SkipWithError("RandomAccessFile::Sync failed.");
      return;
    }

    // TODO(pwnall): Come up with a better strategy for simulating log growth.
    //               This runs out of space on CI.
    ++block_number;
  }

  state.SetBytesProcessed(state.iterations() << block_shift_);
  state.SetItemsProcessed(state.iterations());
}

BENCHMARK_REGISTER_F(VfsBenchmark, LogWrites)->RangeMultiplier(2)->Range(
    4096, 65536);  // Log record size.

}  // namespace berrydb
