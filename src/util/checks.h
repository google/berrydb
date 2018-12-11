// Copyright 2018 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_UTIL_CHECKS_H_
#define BERRYDB_UTIL_CHECKS_H_

#include "berrydb/platform.h"

// Assumptions (BERRYDB_ASSUME*) communicate invariants to the optimizer, and
// enforce the optimizer in builds where DCHECK_IS_ON(). By default, invariants
// should be expressed as assumptions. Unsuitable invariants should be expressed
// as checks (BERRYDB_CHECK*).
//
// Both invariants and checks should prefer the specialized comparison forms
// (_EQ, _NE, _GT, _GE, _LT, _LE) whenever the error message may convey
// additional information about the failure. The vast majority of cases where
// the comparison form doesn't convey additional information is a not-equal
// check against a concrete value -- when BERRYDB_ASSERT_NE(page_id, 0) fails,
// page_id must be 0, so this form doesn't convey additional formation over
// BERRYDB_ASSERT(page_id != 0).
//
// This section will be updated with specific guidance once we have more
// experience around which invariants must be expressed as checks.

#define BERRYDB_CHECK(condition)  DCHECK(condition)
#define BERRYDB_CHECK_EQ(a, b)    DCHECK_EQ((a), (b))
#define BERRYDB_CHECK_NE(a, b)    DCHECK_NE((a), (b))
#define BERRYDB_CHECK_GE(a, b)    DCHECK_GE((a), (b))
#define BERRYDB_CHECK_GT(a, b)    DCHECK_GT((a), (b))
#define BERRYDB_CHECK_LE(a, b)    DCHECK_LE((a), (b))
#define BERRYDB_CHECK_LT(a, b)    DCHECK_LT((a), (b))

#define BERRYDB_ASSUME(condition)  do { \
      BERRYDB_CHECK(condition);         \
      BUILTIN_ASSUME(condition);        \
    } while (0);

#define BERRYDB_ASSUME_EQ(a, b)  do {    \
      BERRYDB_CHECK_EQ((a), (b));        \
      BUILTIN_ASSUME((a) == (b));        \
    } while (0);
#define BERRYDB_ASSUME_NE(a, b)  do {    \
      BERRYDB_CHECK_NE((a), (b));        \
      BUILTIN_ASSUME((a) != (b));        \
    } while (0);

#define BERRYDB_ASSUME_GE(a, b)  do {    \
      BERRYDB_CHECK_GE((a), (b));        \
      BUILTIN_ASSUME((a) >= (b));        \
    } while (0);

#define BERRYDB_ASSUME_GT(a, b)  do {    \
      BERRYDB_CHECK_GT((a), (b));        \
      BUILTIN_ASSUME((a) >  (b));        \
    } while (0);

#define BERRYDB_ASSUME_LE(a, b)  do {    \
      BERRYDB_CHECK_LE((a), (b));        \
      BUILTIN_ASSUME((a) >= (b));        \
    } while (0);

#define BERRYDB_ASSUME_LT(a, b)  do {    \
      BERRYDB_CHECK_LT((a), (b));        \
      BUILTIN_ASSUME((a) >  (b));        \
    } while (0);

#endif  // BERRYDB_UTIL_CHECKS_H_
