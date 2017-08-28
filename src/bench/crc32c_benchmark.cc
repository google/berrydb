// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <random>

#include "benchmark/benchmark.h"
#include "crc32c/crc32c.h"

#include "berrydb/platform.h"

namespace berrydb {

class Crc32cBenchmark : public benchmark::Fixture {
 protected:
   std::mt19937 rnd_;
};

BENCHMARK_DEFINE_F(Crc32cBenchmark, CrcTest)(benchmark::State& state) {
  size_t input_size = state.range(0);
  uint8_t* input = reinterpret_cast<uint8_t*>(Allocate(input_size));
  for (size_t i = 0; i < input_size; ++i)
    input[i] = rnd_();

  while (state.KeepRunning())
    benchmark::DoNotOptimize(crc32c::Crc32c(input, input_size));

  state.SetBytesProcessed(state.iterations() * input_size);

  Deallocate(input, input_size);
}

BENCHMARK_REGISTER_F(Crc32cBenchmark, CrcTest)->Range(4096, 65536);

}  // namespace berrydb
