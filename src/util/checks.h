// Copyright 2018 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_UTIL_CHECKS_H_
#define BERRYDB_UTIL_CHECKS_H_

#include "berrydb/platform.h"

// Assumptions (BERRYDB_ASSUME*) communicate invariants to the optimizer, and
// enforce these assumptions (by crashing on failure) in builds where
// BERRYDB_CHECK_IS_ON(). By default, invariants should be expressed as
// assumptions. Unsuitable invariants should be expressed as checks
// (BERRYDB_CHECK*). BERRYDB_UNREACHABLE() is a better form of
// BERRYDB_ASSUME(false), and may suppress additional compiler warnings or
// enable more optimizations.
//
// Both invariants and checks should prefer the specialized comparison forms
// (_EQ, _NE, _GT, _GE, _LT, _LE) whenever the error message may convey
// additional information about the failure. The vast majority of cases where
// the comparison form doesn't convey additional information is a not-equal
// check against a concrete value -- when BERRYDB_ASSERT_NE(page_id, 0) fails,
// page_id must be 0, so this form doesn't convey additional formation over
// BERRYDB_ASSERT(page_id != 0).
//
// All expressions passed to the macros below may or may not be evaluated, so
// they must be free of side-effects.
//
// This section will be updated with specific guidance once we have more
// experience around which invariants must be expressed as checks.

#define BERRYDB_CHECK_IS_ON()     DCHECK_IS_ON()

#define BERRYDB_CHECK(condition)  DCHECK(condition)
#define BERRYDB_CHECK_EQ(a, b)    DCHECK_EQ((a), (b))
#define BERRYDB_CHECK_NE(a, b)    DCHECK_NE((a), (b))
#define BERRYDB_CHECK_GE(a, b)    DCHECK_GE((a), (b))
#define BERRYDB_CHECK_GT(a, b)    DCHECK_GT((a), (b))
#define BERRYDB_CHECK_LE(a, b)    DCHECK_LE((a), (b))
#define BERRYDB_CHECK_LT(a, b)    DCHECK_LT((a), (b))

#if BERRYDB_CHECK_IS_ON()

#define BERRYDB_ASSUME(condition)  BERRYDB_CHECK(condition)
#define BERRYDB_ASSUME_EQ(a, b)    BERRYDB_CHECK_EQ(a, b)
#define BERRYDB_ASSUME_NE(a, b)    BERRYDB_CHECK_NE(a, b)
#define BERRYDB_ASSUME_GE(a, b)    BERRYDB_CHECK_GE(a, b)
#define BERRYDB_ASSUME_GT(a, b)    BERRYDB_CHECK_GT(a, b)
#define BERRYDB_ASSUME_LE(a, b)    BERRYDB_CHECK_LE(a, b)
#define BERRYDB_ASSUME_LT(a, b)    BERRYDB_CHECK_LT(a, b)

#define BERRYDB_UNREACHABLE() do {  \
      BERRYDB_CHECK(false);         \
      BUILTIN_UNREACHABLE();        \
    } while (0)

#else  // BERRYDB_CHECK_IS_ON()

#define BERRYDB_ASSUME(condition)  BUILTIN_ASSUME(condition)
#define BERRYDB_ASSUME_EQ(a, b)    BUILTIN_ASSUME((a) == (b))
#define BERRYDB_ASSUME_NE(a, b)    BUILTIN_ASSUME((a) != (b))
#define BERRYDB_ASSUME_GE(a, b)    BUILTIN_ASSUME((a) >= (b))
#define BERRYDB_ASSUME_GT(a, b)    BUILTIN_ASSUME((a) >  (b))
#define BERRYDB_ASSUME_LE(a, b)    BUILTIN_ASSUME((a) <= (b))
#define BERRYDB_ASSUME_LT(a, b)    BUILTIN_ASSUME((a) <  (b))
#define BERRYDB_UNREACHABLE()      BUILTIN_UNREACHABLE()

#endif  // BERRYDB_CHECK_IS_ON()

#endif  // BERRYDB_UTIL_CHECKS_H_
