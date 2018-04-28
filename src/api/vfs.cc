// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "berrydb/vfs.h"

namespace berrydb {

Vfs::Vfs() noexcept = default;
Vfs::~Vfs() = default;

BlockAccessFile::BlockAccessFile() noexcept = default;
BlockAccessFile::~BlockAccessFile() = default;
BlockAccessFile::BlockAccessFile(const BlockAccessFile&) noexcept = default;
BlockAccessFile::BlockAccessFile(BlockAccessFile&&) noexcept = default;
BlockAccessFile& BlockAccessFile::operator =(
    const BlockAccessFile&) noexcept = default;
BlockAccessFile& BlockAccessFile::operator =(
    BlockAccessFile&&) noexcept = default;

RandomAccessFile::RandomAccessFile() noexcept = default;
RandomAccessFile::~RandomAccessFile() = default;
RandomAccessFile::RandomAccessFile(const RandomAccessFile&) noexcept = default;
RandomAccessFile::RandomAccessFile(RandomAccessFile&&) noexcept = default;
RandomAccessFile& RandomAccessFile::operator =(
    const RandomAccessFile&) noexcept = default;
RandomAccessFile& RandomAccessFile::operator =(
    RandomAccessFile&&) noexcept = default;

}  // namespace berrydb
