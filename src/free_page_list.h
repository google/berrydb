// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_FREE_PAGE_LIST_H
#define BERRYDB_FREE_PAGE_LIST_H

#include "berrydb/platform.h"

namespace berrydb {

enum class Status : int;
class StoreImpl;
class TransactionImpl;

/** Conceptually, a linked list of free pages in a store.
 *
 * The list of free pages is stored in some of the free pages themselves. The
 * only piece of information that needs to be maintained outside the free pages
 * is the page ID of the list's head page. */
class FreePageList {
 public:
  /** Creates an empty free page list.
   *
   * This constructor initializes transaction free page lists correctly. The
   * free page lists used by store data files need an additional call to
   * set_head_page_id(). */
  constexpr inline FreePageList() noexcept
      : head_page_id_(kInvalidPageId), tail_page_id_(kInvalidPageId) { }
  // The destructor is inlined because it should generate no code.
  inline ~FreePageList() noexcept = default;

  /** Page ID that's guaranteed to be invalid in the context of free page lists.
   *
   * Zero is a good value because the first page in a store file will always be
   * used for the store's header, so it can never come out of AllocPage(), and
   * therefore it can never end up on a free page list. */
  static constexpr size_t kInvalidPageId = 0;

  /** Removes a page from this free list and returns its ID.
   *
   * @param  transaction used to modify the list; must be committed for the
   *                     list modifications to go live
   * @param  page_id     receives the ID of the page that was removed from this
   *                     free list; kInvalidPageId if there are no free pages in
   *                     this list
   * @return             kSuccess if the list operation completed successfully,
   *                     even if the page list is empty; a different status
   *                     means that an error occurred
   */
  Status Pop(TransactionImpl* transaction, size_t* page_id);

  /** Adds a page to this free list.
   *
   * @param  transation used to modify the list; must be committed for the list
   *                    modifications to go live
   * @param  page_id    the ID of the page that will be freed; cannot be
   *                    kInvalidPageId
   * @return            most likely kSuccess or kIoError
   */
  Status Push(TransactionImpl* transaction, size_t page_id);

  /** Merges another list's pages into this list.
   *
   * The merged list's internal state is destroyed, and it may not be used after
   * the merge operation completes.
   *
   * @param  transation used to modify the list; must be committed for the list
   *                    modifications to go live
   * @param other       the list whose contents should be moved into this list
   * @return            most likely kSuccess or kIoError
   */
  Status Merge(TransactionImpl* transaction, FreePageList* other);

  /** The first page in the free list. */
  inline size_t head_page_id() const noexcept {
#if DCHECK_IS_ON()
    DCHECK(!was_merged_);
#endif  // DCHECK_IS_ON()
    return head_page_id_;
  }

  /** The last page in the free list.
   *
   * This method is an implementation detail and is only exposed for testing.
   * This method should not be used for lists where set_head_page_id() was
   * called. */
  inline size_t tail_page_id() const noexcept {
#if DCHECK_IS_ON()
    DCHECK(!was_merged_);
    DCHECK(tail_page_is_defined_);
#endif  // DCHECK_IS_ON()
    return tail_page_id_;
  }

  /** Sets the list's first page.
   *
   * This should be called when the state of a store's free page list is read
   * from disk, which should only happen when a store is opened, and when a
   * transaction is rolled back. Transaction free page lists should never have
   * this method called on them
   */
  inline void set_head_page_id(size_t head_page_id) noexcept {
#if DCHECK_IS_ON()
    DCHECK(!was_merged_);
    tail_page_is_defined_ = false;
#endif  // DCHECK_IS_ON()
    head_page_id_ = head_page_id;
  }

  /** True if this list is not tracking any free pages. */
  inline bool is_empty() const noexcept {
    return head_page_id_ == kInvalidPageId;
  }

#if DCHECK_IS_ON()
  /** True if this list has been was_merged into another list. */
  inline bool was_merged() const noexcept { return was_merged_; }
#endif  // DCHECK_IS_ON()

 private:
  size_t head_page_id_;
  size_t tail_page_id_;

#if DCHECK_IS_ON()
  bool tail_page_is_defined_ = true;
  bool was_merged_ = false;
#endif  // DCHECK_IS_ON()

  /** The offset of the first free entry in a list page.
   *
   * All list operations need to look at this number. It is at the beginning of
   * the page so accessing it requires slightly less code than accessing any
   * other field.
   */
  static constexpr size_t kNextEntryOffset = 0;
  /** The offset of the next list page's id in a list page. */
  static constexpr size_t kNextPageIdOffset = 8;
  /** The offset of the first entry (page id) in a list page. */
  static constexpr size_t kFirstEntryOffset = 16;
  /** The size of each entry (page id) in a free page list page.
   *
   * This is currently set to 8 bytes, needed by page IDs in large databases. */
  static constexpr size_t kEntrySize = 8;

  // TODO(pwnall): Consider / evaluate using base-128 varints instead of 8-byte
  //               ints for page entries. Instead of relying on kEntrySize,
  //               removing an entry would have to scan back for the first byte
  //               without the top bit set.
};

}  // namespace berrydb

#endif  // BERRYDB_FREE_PAGE_LIST_H
