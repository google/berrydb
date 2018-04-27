// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "./free_page_list.h"

#include "berrydb/status.h"
#include "./free_page_list_format.h"
#include "./page_pool.h"
#include "./page.h"
#include "./store_impl.h"
#include "./transaction_impl.h"
#include "./util/span_util.h"

namespace berrydb {

std::tuple<Status, size_t> FreePageList::Pop(TransactionImpl* transaction) {
  DCHECK(transaction != nullptr);
  DCHECK(transaction != transaction->store()->init_transaction());
#if DCHECK_IS_ON()
  DCHECK(!was_merged_);
#endif  // DCHECK_IS_ON()

  if (is_empty()) {
    return {Status::kSuccess, kInvalidPageId};
  }

  StoreImpl* store = transaction->store();
  PagePool* page_pool = store->page_pool();
  Page* head_page;
  Status status = page_pool->StorePage(
      store, head_page_id_, PagePool::kFetchPageData, &head_page);
  if (status != Status::kSuccess)
    return {status, kInvalidPageId};

  // The code below only uses the span size for DCHECKs, and relies on the
  // compiler to optimize the span into a pointer in release mode.
  span<uint8_t> head_page_data =
      head_page->mutable_data(page_pool->page_size());

  size_t next_entry_offset =
      FreePageListFormat::NextEntryOffset(head_page_data);
  if (next_entry_offset == FreePageListFormat::kFirstEntryOffset) {
    // All the entries on this page have been removed. The page itself can be
    // used as a free page.
    uint64_t new_head_page_id64 =
        FreePageListFormat::NextPageId64(head_page_data);
    size_t new_head_page_id = static_cast<size_t>(new_head_page_id64);
    // This check should be optimized out on 64-bit architectures.
    if (new_head_page_id != new_head_page_id64) {
      page_pool->UnpinStorePage(head_page);
      return {Status::kDatabaseTooLarge, kInvalidPageId};
    }

    size_t free_page_id = head_page_id_;
    head_page_id_ = new_head_page_id;
    if (new_head_page_id == kInvalidPageId) {
      tail_page_id_ = kInvalidPageId;

      // It would be correct to set tail_page_is_defined_ to true here. However,
      // the extra code (and complexity in lifecycle) currently has no benefit.
      // Transaction free page lists always have tail_page_is_defined_ set to
      // true, and Store free page lists always have tail_page_is_defined_ set
      // to false. The extra code would get tail_page_is_defined_ set to true
      // for Stores in rare cases. However, Store lists are never merged into
      // other lists.
    }

    page_pool->UnpinStorePage(head_page);
    return {Status::kSuccess, free_page_id};
  }

  next_entry_offset -= FreePageListFormat::kEntrySize;
  if (FreePageListFormat::IsCorruptEntryOffset(
      next_entry_offset, page_pool->page_size())) {
    page_pool->UnpinStorePage(head_page);
    return {Status::kDataCorrupted, kInvalidPageId};
  }

  uint64_t free_page_id64 =
      LoadUint64(head_page_data.subspan(next_entry_offset, 8));
  size_t free_page_id = static_cast<size_t>(free_page_id64);
  // This check should be optimized out on 64-bit architectures.
  if (free_page_id != free_page_id64) {
    page_pool->UnpinStorePage(head_page);
    return {Status::kDatabaseTooLarge, kInvalidPageId};
  }
  transaction->WillModifyPage(head_page);
  FreePageListFormat::SetNextEntryOffset(next_entry_offset, head_page_data);
  page_pool->UnpinStorePage(head_page);
  return {Status::kSuccess, free_page_id};
}

Status FreePageList::Push(TransactionImpl* transaction, size_t page_id) {
  DCHECK(transaction != nullptr);
  DCHECK(transaction != transaction->store()->init_transaction());
  DCHECK(page_id != kInvalidPageId);
#if DCHECK_IS_ON()
  DCHECK(!was_merged_);
#endif  // DCHECK_IS_ON()

  StoreImpl* store = transaction->store();
  PagePool* page_pool = store->page_pool();
  Page* head_page;

  if (head_page_id_ != kInvalidPageId) {
    Status status = page_pool->StorePage(
        store, head_page_id_, PagePool::kFetchPageData, &head_page);
    if (status != Status::kSuccess)
      return status;

    // This code relies on the compiler to optimize away the size from the span.
    span<const uint8_t> head_page_readonly_data =
        head_page->data(page_pool->page_size());

    size_t next_entry_offset =
        FreePageListFormat::NextEntryOffset(head_page_readonly_data);
    if (next_entry_offset < page_pool->page_size()) {
      // We rely on the compiler to optimize out the redundant page size check.
      if (FreePageListFormat::IsCorruptEntryOffset(
          next_entry_offset, page_pool->page_size())) {
        page_pool->UnpinStorePage(head_page);
        return Status::kDataCorrupted;
      }

      // There's room for another entry in the page.
      transaction->WillModifyPage(head_page);
      span<uint8_t> head_page_data =
          head_page->mutable_data(page_pool->page_size());
      StoreUint64(static_cast<uint64_t>(page_id),
                  head_page_data.subspan(next_entry_offset, 8));
      next_entry_offset += FreePageListFormat::kEntrySize;
      FreePageListFormat::SetNextEntryOffset(next_entry_offset, head_page_data);
      page_pool->UnpinStorePage(head_page);
      return Status::kSuccess;
    }

    // The current page is full.
    page_pool->UnpinStorePage(head_page);
  }

  // The page that just freed up will be set up as a list data page, and used to
  // store the list's entries (free page IDs).

  Status status = page_pool->StorePage(
      store, page_id, PagePool::kIgnorePageData, &head_page);
  if (status != Status::kSuccess)
    return status;

  transaction->WillModifyPage(head_page);
  span<uint8_t> head_page_data =
      head_page->mutable_data(page_pool->page_size());
  FreePageListFormat::SetNextEntryOffset(
      FreePageListFormat::kFirstEntryOffset, head_page_data);
  FreePageListFormat::SetNextPageId64(
      static_cast<uint64_t>(head_page_id_), head_page_data);
  page_pool->UnpinStorePage(head_page);

  if (head_page_id_ == kInvalidPageId)
    tail_page_id_ = page_id;
  head_page_id_ = page_id;

  return Status::kSuccess;
}

Status FreePageList::Merge(TransactionImpl *transaction, FreePageList *other) {
  DCHECK(transaction != nullptr);
  DCHECK(transaction != transaction->store()->init_transaction());
  DCHECK(other != nullptr);
#if DCHECK_IS_ON()
  DCHECK(!was_merged_);
  DCHECK(!other->was_merged_);
  DCHECK(other->tail_page_is_defined_);
  other->was_merged_ = true;
#endif  // DCHECK_IS_ON()

  if (other->is_empty())
    return Status::kSuccess;

  StoreImpl* store = transaction->store();
  PagePool* page_pool = store->page_pool();
  Page* head_page;
  Status status = page_pool->StorePage(
      store, head_page_id_, PagePool::kFetchPageData, &head_page);
  if (status != Status::kSuccess)
    return status;
  // Relying on the compiler to optimize the span size away.
  span<const uint8_t> head_page_readonly_data =
      head_page->data(page_pool->page_size());

  size_t other_head_page_id = other->head_page_id_;
  Page* other_head_page;
  status = page_pool->StorePage(
      store, other_head_page_id, PagePool::kFetchPageData, &other_head_page);
  if (status != Status::kSuccess) {
    page_pool->UnpinStorePage(head_page);
    return status;
  }
  // Relying on the compiler to optimize the span size away.
  span<const uint8_t> other_head_page_readonly_data =
      other_head_page->data(page_pool->page_size());

  // Step 1: Each list is a (potentially) non-full page, followed by full pages.
  // Build a chain out of the full pages.

  // The head page ID for the chain of full pages is only used inside this
  // method, and is never returned. Passing around a 64-bit value on a 32-bit
  // system takes less code than detecting 32-bit overflow and erroring out. So,
  // we pass around the value.
  uint64_t full_chain_head_id64 =
      FreePageListFormat::NextPageId64(head_page_readonly_data);

  size_t other_tail_page_id = other->tail_page_id_;
  if (other_tail_page_id != other->head_page_id_) {
    // Build the chain by prepending the other list's full pages to this list's
    // full pages.
    //
    // The pages must be joined in this precise order because the other list is
    // guaranteed to have a well-tracked tail page, whereas this list does not.
    Page* other_tail_page;
    status = page_pool->StorePage(store, other_tail_page_id,
                                  PagePool::kFetchPageData, &other_tail_page);
    if (status != Status::kSuccess) {
      page_pool->UnpinStorePage(other_head_page);
      page_pool->UnpinStorePage(head_page);
      return status;
    }

    transaction->WillModifyPage(other_tail_page);
    span<uint8_t> other_tail_page_data =
        other_tail_page->mutable_data(page_pool->page_size());
    FreePageListFormat::SetNextPageId64(
        full_chain_head_id64, other_tail_page_data);
    page_pool->UnpinStorePage(other_tail_page);

    // The chain's head is now the other list's second page. This page is
    // guaranteed to be full, as well as all the pages after it.
    full_chain_head_id64 =
        FreePageListFormat::NextPageId64(other_head_page_readonly_data);
  }

  // Step 2: The problem has been reduced to merging two list head pages and a
  // chain of full pages. Changing this list's head page ID requires another
  // page write (for the header page), so the code below avoids changing the
  // list head.

  size_t page_size = page_pool->page_size();
  size_t next_entry_offset =
      FreePageListFormat::NextEntryOffset(head_page_readonly_data);
  size_t other_next_entry_offset =
      FreePageListFormat::NextEntryOffset(other_head_page_readonly_data);
  if (FreePageListFormat::IsCorruptEntryOffset(next_entry_offset, page_size) ||
      FreePageListFormat::IsCorruptEntryOffset(other_next_entry_offset,
                                               page_size)) {
    page_pool->UnpinStorePage(other_head_page);
    page_pool->UnpinStorePage(head_page);
    return Status::kDataCorrupted;
  }

  // This list's head page will be modified in both cases below.
  transaction->WillModifyPage(head_page);
  span<uint8_t> head_page_data =
      head_page->mutable_data(page_pool->page_size());

  // Check if we can add all the page IDs in other list's first page (including
  // the first page's ID itself) to our list's first page.
  size_t needed_space_minus_one_entry =
      other_next_entry_offset - FreePageListFormat::kFirstEntryOffset;
  if (next_entry_offset + needed_space_minus_one_entry < page_size) {
    // This list's head page has enough room for all the IDs.
    DCHECK_LE(
        next_entry_offset + needed_space_minus_one_entry +
        FreePageListFormat::kEntrySize, page_size);

    StoreUint64(static_cast<uint64_t>(other_head_page_id),
                head_page_data.subspan(next_entry_offset, 8));
    next_entry_offset += FreePageListFormat::kEntrySize;
    CopySpan(
        other_head_page_readonly_data.subspan(
            FreePageListFormat::kFirstEntryOffset,
            needed_space_minus_one_entry),
        head_page_data.subspan(next_entry_offset,
                               needed_space_minus_one_entry));
    next_entry_offset += needed_space_minus_one_entry;
  } else {
    // This list's head page cannot accomodate all page IDs. Move IDs from this
    // list's head page to fill up the other list's head page, and then chain
    // the other list's head page to this list's head page.
    transaction->WillModifyPage(other_head_page);
    span<uint8_t> other_head_page_data =
        other_head_page->mutable_data(page_pool->page_size());

    // DCHECK failure implies that the data corruption check above is broken.
    DCHECK_LE(other_next_entry_offset, page_size);
    size_t empty_space = page_size - other_next_entry_offset;
    // DCHECK failure implies that the page IDs in the two head pages do fit in
    // a single page, so the capacity check above is broken.
    DCHECK_LE(empty_space, next_entry_offset);
    size_t new_next_entry_offset = next_entry_offset - empty_space;
    CopySpan(
        head_page_data.subspan(new_next_entry_offset, empty_space),
        other_head_page_data.subspan(other_next_entry_offset, empty_space));
    FreePageListFormat::SetNextEntryOffset(page_size, other_head_page_data);
    next_entry_offset = new_next_entry_offset;

    FreePageListFormat::SetNextPageId64(
        static_cast<uint64_t>(other_head_page_id), head_page_data);
  }

  // next_entry_offset is changed in both cases above, stored once here.
  FreePageListFormat::SetNextEntryOffset(next_entry_offset, head_page_data);

  page_pool->UnpinStorePage(other_head_page);
  page_pool->UnpinStorePage(head_page);
  return Status::kSuccess;
}

constexpr size_t FreePageList::kInvalidPageId;

}  // namespace berrydb
