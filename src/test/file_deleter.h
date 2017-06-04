// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_TEST_FILE_DELETER_H_
#define BERRYDB_TEST_FILE_DELETER_H_

#include <string>

namespace berrydb {

/** A wrapper that ensures a temporary file is deleted before/after tests.
 *
 * The wrapper calls DeleteFile() on the wrapped path when it is constructed and
 * destroyed. The wrapper should be used as a member in test fixture classes.
 * Proper usage will ensure that tests don't leave temporary files behind, and
 * are not impacted by any temporary file that may have been left behind.
 */
class FileDeleter {
 public:
  /** Creates a deleter for a temporary file. */
  FileDeleter(const std::string& path);
  ~FileDeleter();

  /** Path to the temporary file. */
  inline const std::string& path() { return path_; }

 private:
  const std::string path_;
};

}  // namespace berrydb

#endif  // BERRYDB_TEST_FILE_DELETER_H_
