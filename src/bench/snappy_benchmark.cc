// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <random>

#include "benchmark/benchmark.h"
#include "snappy.h"

#include "berrydb/platform.h"

namespace berrydb {

class SnappyBenchmark : public benchmark::Fixture {
 protected:
   std::mt19937 rnd_;
};

BENCHMARK_DEFINE_F(SnappyBenchmark, CompressionTest)(benchmark::State& state) {
  size_t input_size = static_cast<size_t>(state.range(0));
  size_t output_buffer_size = snappy::MaxCompressedLength(input_size);
  uint8_t* input = reinterpret_cast<uint8_t*>(Allocate(input_size));
  uint8_t* output = reinterpret_cast<uint8_t*>(Allocate(output_buffer_size));
  for (size_t i = 0; i < input_size; ++i)
    input[i] = static_cast<uint8_t>(rnd_());

  for (auto _ : state) {
    size_t output_size;
    snappy::RawCompress(
        reinterpret_cast<char*>(input), input_size,
        reinterpret_cast<char*>(output), &output_size);
    DCHECK_LE(output_size, output_buffer_size);
  }

  state.SetBytesProcessed(state.iterations() * input_size);

  Deallocate(output, output_buffer_size);
  Deallocate(input, input_size);
}

BENCHMARK_REGISTER_F(SnappyBenchmark, CompressionTest)->Range(4096, 65536);

}  // namespace berrydb
