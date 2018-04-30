// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_PLATFORM_DCHECK_H_
#define BERRYDB_PLATFORM_DCHECK_H_

#include <cassert>

#include "berrydb/platform/config.h"

#if defined(BERRYDB_PLATFORM_BUILT_WITH_GLOG)

#include <glog/logging.h>

#else  // !defined(BERRYDB_PLATFORM_BUILT_WITH_GLOG)

/** Google-style assert.
 *
 * Assertions must be enabled when the DCHECK_IS_ON() macro evaluates to true,
 * and disabled when the DCHECK_IS_ON() macro evaluates to false.
 */
#if !defined(DCHECK)
#define DCHECK(condition) assert(condition)
#endif  // !defined(DCHECK)

#if !defined(DCHECK_IS_ON)
#if defined(NDEBUG) && !defined(DCHECK_ALWAYS_ON)
#define DCHECK_IS_ON() 0
#else  // !(defined(NDEBUG) && !defined(DCHECK_ALWAYS_ON))
#define DCHECK_IS_ON() 1
#endif  // defined(NDEBUG) && !defined(DCHECK_ALWAYS_ON)
#endif  // !defined(DCHECK_IS_ON)

/** Convenience wrappers over DCHECK. */
#if !defined(DCHECK_EQ)
#define DCHECK_EQ(a, b) DCHECK((a) == (b))
#endif  // !defined(DCHECK_EQ)
#if !defined(DCHECK_NE)
#define DCHECK_NE(a, b) DCHECK((a) != (b))
#endif  // !defined(DCHECK_NE)
#if !defined(DCHECK_GE)
#define DCHECK_GE(a, b) DCHECK((a) >= (b))
#endif  // !defined(DCHECK_GE)
#if !defined(DCHECK_GT)
#define DCHECK_GT(a, b) DCHECK((a) > (b))
#endif  // !defined(DCHECK_GT)
/** Convenience wrapper over DCHECK. */
#if !defined(DCHECK_LE)
#define DCHECK_LE(a, b) DCHECK((a) <= (b))
#endif  // !defined(DCHECK_LE)
/** Convenience wrapper over DCHECK. */
#if !defined(DCHECK_LT)
#define DCHECK_LT(a, b) DCHECK((a) < (b))
#endif  // !defined(DCHECK_LT)

#endif  // defined(BERRYDB_PLATFORM_BUILT_WITH_GLOG)

/** Silences compiler warnings about unused variables.
 *
 * This is generally necessary in code that uses DCHECK_IS_ON(). */
#if !defined(UNUSED)
#define UNUSED(x) (void)(x)
#endif  // !defined(UNUSED)

#endif  // BERRYDB_PLATFORM_DCHECK_H_
