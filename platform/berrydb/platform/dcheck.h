// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_PLATFORM_DCHECK_H_
#define BERRYDB_PLATFORM_DCHECK_H_

#include <cassert>

// Google-style assert().
#define DCHECK(condition)   assert(condition)
#define DCHECK_LE(a, b)  DCHECK((a) <= (b))

#endif  // BERRYDB_PLATFORM_DCHECK_H_
