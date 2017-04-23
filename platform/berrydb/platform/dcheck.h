// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_PLATFORM_DCHECK_H_
#define BERRYDB_PLATFORM_DCHECK_H_

// Embedders who use google/glog can #include <glog/logging.h> and get better
// failure messages.

#include <cassert>

/** Google-style assert.
 *
 * Assertions must be enabled when the DCHECK_IS_ON() macro evaluates to true,
 * and disabled when the DCHECK_IS_ON() macro evaluates to false.
 */
#define DCHECK(condition)   assert(condition)

#if defined(NDEBUG) && !defined(DCHECK_ALWAYS_ON)
#define DCHECK_IS_ON() 0
#else
#define DCHECK_IS_ON() 1
#endif

/** Convenience wrapper over DCHECK. */
#define DCHECK_EQ(a, b)  DCHECK((a) == (b))
/** Convenience wrapper over DCHECK. */
#define DCHECK_NE(a, b)  DCHECK((a) != (b))
/** Convenience wrapper over DCHECK. */
#define DCHECK_GT(a, b)  DCHECK((a) > (b))
/** Convenience wrapper over DCHECK. */
#define DCHECK_LE(a, b)  DCHECK((a) <= (b))

#endif  // BERRYDB_PLATFORM_DCHECK_H_
