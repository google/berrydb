// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "berrydb/vfs.h"

namespace berrydb {

Vfs::Vfs() noexcept = default;
Vfs::~Vfs() = default;

BlockAccessFile::BlockAccessFile() noexcept = default;
BlockAccessFile::~BlockAccessFile() = default;
BlockAccessFile::BlockAccessFile(
    const BlockAccessFile& other) noexcept = default;
BlockAccessFile::BlockAccessFile(
    BlockAccessFile&& other) noexcept = default;
BlockAccessFile& BlockAccessFile::operator =(
    const BlockAccessFile& other) noexcept = default;
BlockAccessFile& BlockAccessFile::operator =(
    BlockAccessFile&& other) noexcept = default;

RandomAccessFile::RandomAccessFile() noexcept = default;
RandomAccessFile::~RandomAccessFile() = default;
RandomAccessFile::RandomAccessFile(
    const RandomAccessFile& other) noexcept = default;
RandomAccessFile::RandomAccessFile(
    RandomAccessFile&& other) noexcept = default;
RandomAccessFile& RandomAccessFile::operator =(
    const RandomAccessFile& other) noexcept = default;
RandomAccessFile& RandomAccessFile::operator =(
    RandomAccessFile&& other) noexcept = default;

}  // namespace berrydb
