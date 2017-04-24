// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "./store_impl.h"

#include "berrydb/options.h"
#include "berrydb/vfs.h"
#include "./pool_impl.h"

namespace berrydb {

static_assert(std::is_standard_layout<StoreImpl>::value,
    "StoreImpl must be a standard layout type so its public API can be "
    "exposed cheaply");

StoreImpl* StoreImpl::Create(
      BlockAccessFile* data_file, PagePool* page_pool,
      const StoreOptions& options) {
  void* heap_block = Allocate(sizeof(StoreImpl));
  StoreImpl* store = new (heap_block) StoreImpl(data_file, page_pool, options);
  DCHECK_EQ(heap_block, static_cast<void*>(store));
  return store;
}

StoreImpl::StoreImpl(
    BlockAccessFile* data_file, PagePool* page_pool,
    const StoreOptions& options)
    : api_(), data_file_(data_file), page_pool_(page_pool),
      page_shift_(page_pool->page_shift()) {
  DCHECK(data_file != nullptr);
  DCHECK(page_pool != nullptr);
}

TransactionImpl* StoreImpl::CreateTransaction() {
  return nullptr;
}

Status StoreImpl::Close() {
  return Status::kIoError;
}

Status StoreImpl::ReadPage(Page* page) {
  DCHECK_EQ(this, page->store());
  DCHECK(!page->is_dirty());

  size_t file_offset = page->page_id() << page_shift_;
  size_t page_size = 1 << page_shift_;
  return data_file_->Read(file_offset, page_size, page->data());
}

Status StoreImpl::WritePage(Page* page) {
  DCHECK_EQ(this, page->store());

  size_t file_offset = page->page_id() << page_shift_;
  size_t page_size = 1 << page_shift_;
  return data_file_->Write(page->data(), file_offset, page_size);
}

}  // namespace berrydb
