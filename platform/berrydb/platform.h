// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * This directory serves a dual purpose as both a specification of the BerryDB
 * platform interface, and a BerryDB platform implementation based on the
 * standard C++ library.
 *
 * This file is included via the "berrydb/platform.h". BerryDB embedders can use
 * a different platform implementation by removing the stdcpp_platform directory
 * from BerryDB's include path, and replacing it with a directory that
 * contains a custom berrydb/platform.h.
 */

#ifndef BERRYDB_PLATFORM_H_
#define BERRYDB_PLATFORM_H_

#include "berrydb/platform/config.h"

#include "./platform/alloc.h"
#include "./platform/dcheck.h"
#include "./platform/endianness.h"
#include "./platform/hashing.h"

namespace berrydb {}  // namespace berrydb

#endif  // BERRYDB_PLATFORM_H_
