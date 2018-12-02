// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Normalizes namespacing for standard C++ integral types.
//
// The C++ standard specifies that <cstddef> and <cstdint> introduce types such
// as uint8_t in the std:: namespace, and that the headers may also introduce
// these types in the global namespace. In practice, all known C++ standard
// library implementations introduce the integral types in both namespaces, so
// it's impossible to catch a naked (not std:: qualified) reference to these
// integral types.
//
// BerryDB side-steps this issue by aliasing all the types it uses into the
// berrydb:: namespace. As all the BerryDB code lives in the berrydb::
// namespace, #including this headers allows the rest of the BerryDB code to
// safely use naked references to integral types.

#ifndef BERRYDB_INCLUDE_BERRYDB_TYPES_H_
#define BERRYDB_INCLUDE_BERRYDB_TYPES_H_

#include <cstddef>
#include <cstdint>

namespace berrydb {

// Preferred type for representing bytes that make up binary data.
//
// Prefer to char and unsigned char for most situations.
//
// char is acceptable for values that are passed through between the user and
// platform APIs, such as filesystem and logging.
using std::uint8_t;

// Preferred type for representing sizes of containers that reside in RAM.
//
// Prefer to uint64_t for values known to represent the sizes of RAM-bound
// containers, as size_t results in better code on 32-bit platforms. uint8_t
// buffers (such as database pages) also count as RAM containers, so page sizes
// are represented using size_t.
//
// For similar reasons, prefer to uint64_t for values that end up being passed
// to platform APIs as size_t arguments.
using std::size_t;

// Numbers that are 64-bit wide on all computers.
//
// By default, the sizes of on-disk data structures should be 64-bit numbers, to
// accomodate for large databases. For example, the total number of pages in a
// database should be 64-bit.
using std::uint64_t;

// Numbers that are 32-bit wide on all computers.
using std::uint32_t;

// Only used for doing bit operations on the memory address in a pointer.
//
// This type should be limited to very specialized use cases. For example,
// computing the hash value of a pointer requires treating it as a number.
using std::uintptr_t;

}  // namespace berrydb

#endif  // BERRYDB_INCLUDE_BERRYDB_TYPES_H_
