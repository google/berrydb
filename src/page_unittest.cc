// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "./page.h"

#include "gtest/gtest.h"

#include "./page_pool.h"

namespace berrydb {

TEST(PageTest, CreateRelease) {
  PagePool page_pool(12, 42);

  Page* page = Page::Create(&page_pool);
#if DCHECK_IS_ON()
  EXPECT_EQ(nullptr, page->store());
  EXPECT_EQ(&page_pool, page->page_pool());
#endif  // DCHECK_IS_ON()

  page->RemovePin();
  EXPECT_TRUE(page->IsUnpinned());

  page->Release(&page_pool);
}

TEST(PageTest, Pinning) {
  PagePool page_pool(12, 42);

  Page* page = Page::Create(&page_pool);
  EXPECT_FALSE(page->IsUnpinned());
  page->RemovePin();
  EXPECT_TRUE(page->IsUnpinned());

  page->AddPin();
  EXPECT_FALSE(page->IsUnpinned());

  page->RemovePin();
  EXPECT_TRUE(page->IsUnpinned());

  page->AddPin();
  page->AddPin();
  EXPECT_FALSE(page->IsUnpinned());

  page->RemovePin();
  EXPECT_FALSE(page->IsUnpinned());
  page->RemovePin();
  EXPECT_TRUE(page->IsUnpinned());

  page->Release(&page_pool);
}

}  // namespace berrydb
