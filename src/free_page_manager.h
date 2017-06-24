// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_FREE_PAGE_MANAGER_H
#define BERRYDB_FREE_PAGE_MANAGER_H

/** Tracks the free pages in a store's data file.
 *
 * Each store has a free page manager (a.k.a. free space manager). Pages that
 * become empty after data is deleted cannot be immediately returned to the
 * underlying filesystem, because a store's data file is a continuous sequence
 * of pages. Instead, the page IDs for free pages are stored in a list, so they
 * can be reused. The free page list entries are stored in pages that are
 * exclusively allocated for this purpose.
 */
class FreePageManager {

};

#endif  // BERRYDB_FREE_PAGE_MANAGER_H
