// Copyright 2018 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_PINNED_PAGE_H_
#define BERRYDB_PINNED_PAGE_H_

#include "./page.h"
#include "./page_pool.h"
#include "./util/checks.h"

namespace berrydb {

// Utility class that releases a pin to a page when it goes out of scope.
class PinnedPage {
 public:
  /** Constructs a wrapper for a Page pin.
   *
   * The caller must own a pin to the Page. The pin ownership is passed to the
   * newly created instance. The Page will be unpinned when the instance goes
   * out of scope.
   *
   * @param page      the Page wrapped by this instance
   * @param page_pool the PagePool that the wrapped Page belongs to
   */
  inline constexpr PinnedPage(Page* page, PagePool* page_pool) noexcept
      : page_(page), page_pool_(page_pool) {
    BERRYDB_ASSUME(page != nullptr);
    BERRYDB_ASSUME(page_pool != nullptr);
    BERRYDB_ASSUME(!page->IsUnpinned());
#if BERRYDB_CHECK_IS_ON()
    // BERRYDB_CHECK_EQ doesn't currently work inside constexpr.
    BERRYDB_CHECK(page->page_pool() == page_pool);
#endif  // BERRYDB_CHECK_IS_ON()
  }

  inline ~PinnedPage() { page_pool_->UnpinStorePage(page_); }

  PinnedPage(const PinnedPage&) = delete;
  PinnedPage(PinnedPage&&) = delete;
  PinnedPage& operator=(const PinnedPage&) = delete;
  PinnedPage& operator=(PinnedPage&&) = delete;

  /** Convenience proxy to Page::data(). */
  inline span<const uint8_t> data() const noexcept {
    return page_->data(page_pool_->page_size());
  }

  /** Convenience proxy to Page::mutable_data(). */
  inline span<uint8_t> mutable_data() const noexcept {
    return page_->mutable_data(page_pool_->page_size());
  }

  /** Returns the wrapped Page instance. */
  inline constexpr Page* get() const noexcept { return page_; }

  /** Convenience operator for accessing the wrapped Page. */
  inline constexpr Page& operator *() const noexcept { return *page_; }

  /** Convenience operator for accessing the wrapped Page's methods. */
  inline constexpr Page* operator ->() const noexcept { return page_; }

 private:
  Page* const page_;
  PagePool* const page_pool_;
};

}  // namespace berrydb

#endif  // BERRYDB_PINNED_PAGE_H_
