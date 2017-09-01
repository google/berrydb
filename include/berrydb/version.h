// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_INCLUDE_BERRYDB_VERSION_H_
#define BERRYDB_INCLUDE_BERRYDB_VERSION_H_

namespace berrydb {

// API major version number. Incremented for backwards-incompatible API changes.
extern constexpr const unsigned kVersionMajor = 0;

// API minor version number. Incremented for backwards-compatible API changes.
extern constexpr const unsigned kVersionMinor = 1;

// API minor version number. Incremented for bugfixes.
extern constexpr const unsigned kVersionPatch = 0;

};  // namespace berrydb

#endif  // BERRYDB_INCLUDE_BERRYDB_VERSION_H_
