// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "./free_page_list.h"

#include <cstring>

#include "berrydb/status.h"
#include "./page_pool.h"
#include "./page.h"
#include "./store_impl.h"
#include "./transaction_impl.h"

namespace berrydb {

Status FreePageList::Pop(TransactionImpl* transaction, size_t* page_id) {
  DCHECK(transaction != nullptr);
  DCHECK(transaction != transaction->store()->init_transaction());
  DCHECK(page_id != nullptr);
#if DCHECK_IS_ON()
  DCHECK(!was_merged_);
#endif  // DCHECK_IS_ON()

  if (is_empty()) {
    *page_id = kInvalidPageId;
    return Status::kSuccess;
  }

  StoreImpl* store = transaction->store();
  PagePool* page_pool = store->page_pool();
  Page* head_page;
  Status status = page_pool->StorePage(
      store, head_page_id_, PagePool::kFetchPageData, &head_page);
  if (status != Status::kSuccess)
    return status;

  uint8_t* head_page_data = head_page->data();
  // Casting is safe because the page size must be smaller than size_t. The next
  // entry offset must be at most page_size, so it must be smaller than size_t.
  size_t next_entry_offset =
      static_cast<size_t>(LoadUint64(head_page_data + kNextEntryOffset));
  if (next_entry_offset == kFirstEntryOffset) {
    // All the entries on this page have been removed. The page itself can be
    // used as a free page.
    uint64_t next_page_id64 = LoadUint64(head_page_data + kNextPageIdOffset);
    size_t next_page_id = static_cast<size_t>(next_page_id64);
    // This check should be optimized out on 64-bit architectures.
    if (next_page_id != next_page_id64)
      return Status::kDatabaseTooLarge;

    *page_id = head_page_id_;
    head_page_id_ = next_page_id;

    page_pool->UnpinStorePage(head_page);
    return Status::kSuccess;
  }

  next_entry_offset -= kEntrySize;
  // Check for store data file corruption to avoid an illegal memory access.
  static_assert(
      (kEntrySize & (kEntrySize - 1)) == 0,
      "kEntrySize must be a power of two for bit masking tricks to work");
  if (next_entry_offset >= page_pool->page_size() ||
      (next_entry_offset & (kEntrySize - 1)) != 0) {
    page_pool->UnpinStorePage(head_page);
    return Status::kDataCorrupted;
  }

  DCHECK_GE(next_entry_offset, kFirstEntryOffset);

  // NOTE: We rely on the compiler to optimize the multiplication to a shift.
  uint64_t free_page_id64 = LoadUint64(head_page_data + next_entry_offset);
  size_t free_page_id = static_cast<size_t>(free_page_id64);
  // This check should be optimized out on 64-bit architectures.
  if (free_page_id != free_page_id64)
      return Status::kDatabaseTooLarge;
  head_page->MarkDirty();
  StoreUint64(
      static_cast<uint64_t>(next_entry_offset),
      head_page_data + kNextEntryOffset);
  *page_id = free_page_id;
  page_pool->UnpinStorePage(head_page);
  return Status::kSuccess;
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
  Status status = page_pool->StorePage(
      store, head_page_id_, PagePool::kFetchPageData, &head_page);
  if (status != Status::kSuccess)
    return status;

  uint8_t* head_page_data = head_page->data();
  // Casting is safe because the page size must be smaller than size_t. The next
  // entry offset must be at most page_size, so it must be smaller than size_t.
  size_t next_entry_offset =
      static_cast<size_t>(LoadUint64(head_page_data + kNextEntryOffset));
  if (next_entry_offset < page_pool->page_size()) {
    // Check for store data file corruption to avoid an illegal memory access.
    static_assert(
        (kEntrySize & (kEntrySize - 1)) == 0,
        "kEntrySize must be a power of two for bit masking tricks to work");
    if ((next_entry_offset & (kEntrySize - 1)) != 0) {
      page_pool->UnpinStorePage(head_page);
      return Status::kDataCorrupted;
    }

    // There's room for another entry in the page.
    head_page->MarkDirty();
    StoreUint64(
        static_cast<uint64_t>(next_entry_offset + kEntrySize),
        head_page_data + kNextEntryOffset);
    StoreUint64(
        static_cast<uint64_t>(page_id),
        head_page_data + next_entry_offset);
    page_pool->UnpinStorePage(head_page);
    return Status::kSuccess;
  }

  // The current page is full. The page that just freed up will be used to store
  // free page IDs.
  page_pool->UnpinStorePage(head_page);

  status = page_pool->StorePage(
      store, page_id, PagePool::kIgnorePageData, &head_page);
  if (status != Status::kSuccess)
    return status;

  head_page->MarkDirty();
  head_page_data = head_page->data();
  StoreUint64(
      static_cast<uint64_t>(kFirstEntryOffset),
      head_page_data + kNextEntryOffset);
  StoreUint64(
      static_cast<uint64_t>(head_page_id_),
      head_page_data + kNextPageIdOffset);
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
  uint8_t* head_page_data = head_page->data();

  size_t other_head_page_id = other->head_page_id_;
  Page* other_head_page;
  status = page_pool->StorePage(
      store, other_head_page_id, PagePool::kFetchPageData, &other_head_page);
  if (status != Status::kSuccess) {
    page_pool->UnpinStorePage(head_page);
    return status;
  }
  uint8_t* other_head_page_data = other_head_page->data();

  // Step 1: Each list is a (potentially) non-full page, followed by full pages.
  // Build a chain out of the full pages.

  // The head page ID for the chain of full pages is only used inside this
  // method, and is never returned. Passing around a 64-bit value on a 32-bit
  // system takes less code than detecting 32-bit overflow and erroring out. So,
  // we pass around the value.
  uint64_t full_chain_head_id64 =
      LoadUint64(head_page_data + kNextPageIdOffset);

  size_t other_tail_page_id = other->tail_page_id_;
  if (other_tail_page_id != other->head_page_id_) {
    // Build the chain by prepending the other list's full pages to this list's
    // full pages.
    //
    // The pages must be joined in this precise order because the other list is
    // guaranteed to have a well-tracked tail page, whereas this list does not.
    Page* other_tail_page;
    Status status = page_pool->StorePage(
        store, other_tail_page_id, PagePool::kFetchPageData, &other_tail_page);
    if (status != Status::kSuccess) {
      page_pool->UnpinStorePage(other_head_page);
      page_pool->UnpinStorePage(head_page);
      return status;
    }

    other_tail_page->MarkDirty();
    StoreUint64(
        full_chain_head_id64, other_tail_page->data() + kNextPageIdOffset);
    page_pool->UnpinStorePage(other_tail_page);

    // The chain's head is now the other list's second page. This page is
    // guaranteed to be full, as well as all the pages after it.
    full_chain_head_id64 = LoadUint64(
        other_head_page_data + kNextPageIdOffset);
  }

  // Step 2: The problem has been reduced to merging two list head pages and a
  // chain of full pages. Changing this list's head page ID requires another
  // page write (for the header page), so the code below avoids changing the
  // list head.

  // Casting is safe because the page size must be smaller than size_t. The next
  // entry offset must be at most page_size, so it must be smaller than size_t.
  size_t page_size = page_pool->page_size();
  size_t next_entry_offset =
      static_cast<size_t>(LoadUint64(head_page_data + kNextEntryOffset));
  size_t other_next_entry_offset =
      static_cast<size_t>(LoadUint64(other_head_page_data + kNextEntryOffset));
  // Check for store data file corruption to avoid illegal memory accesses.
  static_assert(
      (kEntrySize & (kEntrySize - 1)) == 0,
      "kEntrySize must be a power of two for bit masking tricks to work");
  if (next_entry_offset >= page_size ||
      (next_entry_offset & (kEntrySize - 1)) != 0 ||
      other_next_entry_offset >= page_size ||
      (other_next_entry_offset & (kEntrySize - 1)) != 0) {
    page_pool->UnpinStorePage(other_head_page);
    page_pool->UnpinStorePage(head_page);
    return Status::kDataCorrupted;
  }

  // This list's head page will be modified in both cases below.
  head_page->MarkDirty();

  // Check if we can add all the page IDs in other list's first page (including
  // the first page's ID itself) to our list's first page.
  size_t needed_space_minus_one_entry =
      other_next_entry_offset - kFirstEntryOffset;
  if (next_entry_offset + needed_space_minus_one_entry < page_size) {
    // This list's head page has enough room for all the IDs.
    DCHECK_LE(
        next_entry_offset + needed_space_minus_one_entry + kEntrySize,
        page_size);

    StoreUint64(
        static_cast<uint64_t>(other_head_page_id),
        head_page_data + next_entry_offset);
    next_entry_offset += kEntrySize;
    std::memcpy(
        head_page_data + next_entry_offset,
        other_head_page_data + kFirstEntryOffset, needed_space_minus_one_entry);
    next_entry_offset += needed_space_minus_one_entry;
  } else {
    // This list's head page cannot accomodate all page IDs. Move IDs from this
    // list's head page to fill up the other list's head page, and then chain
    // the other list's head page to this list's head page.
    other_head_page->MarkDirty();

    // DCHECK failure implies that the data corruption check above is broken.
    DCHECK_LE(other_next_entry_offset, page_size);
    size_t empty_space = page_size - other_next_entry_offset;
    // DCHECK failure implies that the page IDs in the two head pages do fit in
    // a single page, so the capacity check above is broken.
    DCHECK_LE(empty_space, next_entry_offset);
    size_t new_next_entry_offset = next_entry_offset - empty_space;
    std::memcpy(
        other_head_page_data + other_next_entry_offset,
        head_page_data + new_next_entry_offset, empty_space);
    StoreUint64(
        static_cast<uint64_t>(page_size),
        other_head_page_data + kNextEntryOffset);
    next_entry_offset = new_next_entry_offset;

    StoreUint64(
        static_cast<uint64_t>(other_head_page_id),
        head_page_data + kNextPageIdOffset);
  }

  // next_entry_offset is changed in both cases above, stored once here.
  StoreUint64(
      static_cast<uint64_t>(next_entry_offset),
      head_page_data + kNextEntryOffset);

  page_pool->UnpinStorePage(other_head_page);
  page_pool->UnpinStorePage(head_page);
  return Status::kSuccess;
}

}  // namespace berrydb
