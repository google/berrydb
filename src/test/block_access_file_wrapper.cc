// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "./block_access_file_wrapper.h"

#include "berrydb/status.h"
#include "../util/checks.h"

namespace berrydb {

BlockAccessFileWrapper::BlockAccessFileWrapper(BlockAccessFile* file)
    : file_(file), access_error_(Status::kSuccess) { }

BlockAccessFileWrapper::~BlockAccessFileWrapper() {
  if (!wrapped_file_is_closed_)
    file_->Close();
}

Status BlockAccessFileWrapper::Read(size_t offset, span<uint8_t> buffer) {
  BERRYDB_ASSUME(!is_closed_);
  if (access_error_ != Status::kSuccess)
    return access_error_;
  return file_->Read(offset, buffer);
}

Status BlockAccessFileWrapper::Write(span<const uint8_t> data, size_t offset) {
  BERRYDB_ASSUME(!is_closed_);
  if (access_error_ != Status::kSuccess)
    return access_error_;
  return file_->Write(data, offset);
}

Status BlockAccessFileWrapper::Sync() {
  BERRYDB_ASSUME(!is_closed_);
  if (access_error_ != Status::kSuccess)
    return access_error_;
  return file_->Sync();
}

Status BlockAccessFileWrapper::Lock() {
  BERRYDB_ASSUME(!is_closed_);
  if (access_error_ != Status::kSuccess)
    return access_error_;
  return file_->Lock();
}


Status BlockAccessFileWrapper::Close() {
  BERRYDB_ASSUME(!is_closed_);
  is_closed_ = true;

  if (access_error_ != Status::kSuccess)
    return access_error_;

  wrapped_file_is_closed_ = true;
  return file_->Close();
}

}  // namespace berrydb
