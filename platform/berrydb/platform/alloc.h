// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_PLATFORM_ALLOC_H_
#define BERRYDB_PLATFORM_ALLOC_H_

// Embedders who use custom memory allocators will want to replace the functions
// below. The interface is inspired from std::pmr::memory_resource.

#include <cstddef>
#include <cstdint>
#include <cstdlib>

#include "./dcheck.h"
#include "./types.h"

namespace berrydb {

/**
 * Dynamically allocates memory.
 *
 * @param bytes guaranteed to be positive
 * @return a pointer guaranteed to be aligned to size_t
 */
inline void* Allocate(std::size_t size_in_bytes) {
  DCHECK(size_in_bytes > 0);

#if DCHECK_IS_ON()
  void* heap_block = std::malloc(size_in_bytes + sizeof(size_t));

  *static_cast<size_t*>(heap_block) = size_in_bytes;
  void* data = static_cast<void*>(static_cast<size_t*>(heap_block) + 1);
#else  // DCHECK_IS_ON()
  void* data = std::malloc(size_in_bytes);
#endif  // DCHECK_IS_ON()

  DCHECK_EQ(reinterpret_cast<uintptr_t>(data) & (sizeof(size_t) - 1), 0);
  return data;
}

/**
 * Releases memory that was previously allocated with Allocate().
 *
 * @param data  result of a previous call to Allocate(bytes)
 * @param bytes must match the value passed to the Allocate() call
 */
inline void Deallocate(void* data, std::size_t size_in_bytes) {
  DCHECK(size_in_bytes > 0);
  DCHECK(data != nullptr);
  DCHECK_EQ(reinterpret_cast<uintptr_t>(data) & (sizeof(size_t) - 1), 0);

#if DCHECK_IS_ON()
  void* heap_block = static_cast<void*>(static_cast<size_t*>(data) - 1);

  DCHECK_EQ(*static_cast<size_t*>(heap_block), size_in_bytes);
#else  // DCHECK_IS_ON()
  void* heap_block = data;
#endif  // DCHECK_IS_ON()

  free(heap_block);
  UNUSED(size_in_bytes);
}

}  // namespace berrydb

#endif  // BERRYDB_PLATFORM_ALLOC_H_
