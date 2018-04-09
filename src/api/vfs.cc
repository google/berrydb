// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "berrydb/vfs.h"
#include "../debug.h"

namespace berrydb {

Vfs::Vfs() noexcept = default;
Vfs::Vfs(const Vfs& other UNUSED) noexcept = default;
Vfs::Vfs(Vfs&& other UNUSED) noexcept = default;
Vfs& Vfs::operator =(const Vfs& other UNUSED) noexcept = default;
Vfs& Vfs::operator =(Vfs&& other UNUSED) noexcept = default;

BlockAccessFile::BlockAccessFile() noexcept = default;
BlockAccessFile::~BlockAccessFile() = default;
BlockAccessFile::BlockAccessFile(
    const BlockAccessFile& other UNUSED) noexcept = default;
BlockAccessFile::BlockAccessFile(
    BlockAccessFile&& other UNUSED) noexcept = default;
BlockAccessFile& BlockAccessFile::operator =(
    const BlockAccessFile& other UNUSED) noexcept = default;
BlockAccessFile& BlockAccessFile::operator =(
    BlockAccessFile&& other UNUSED) noexcept = default;

RandomAccessFile::RandomAccessFile() noexcept = default;
RandomAccessFile::~RandomAccessFile() = default;
RandomAccessFile::RandomAccessFile(
    const RandomAccessFile& other UNUSED) noexcept = default;
RandomAccessFile::RandomAccessFile(
    RandomAccessFile&& other UNUSED) noexcept = default;
RandomAccessFile& RandomAccessFile::operator =(
    const RandomAccessFile& other UNUSED) noexcept = default;
RandomAccessFile& RandomAccessFile::operator =(
    RandomAccessFile&& other UNUSED) noexcept = default;

}  // namespace berrydb
