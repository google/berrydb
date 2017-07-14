// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "./free_page_list_format.h"

namespace berrydb {

constexpr size_t FreePageListFormat::kNextEntryOffset;
constexpr size_t FreePageListFormat::kNextPageIdOffset;
constexpr size_t FreePageListFormat::kFirstEntryOffset;
constexpr size_t FreePageListFormat::kEntrySize;

}  // namespace berrydb
