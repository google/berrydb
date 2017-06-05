// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "berrydb/pool.h"

#include <string>

#include "gtest/gtest.h"

#include "berrydb/options.h"
#include "berrydb/status.h"
#include "berrydb/store.h"
#include "berrydb/vfs.h"
#include "../test/file_deleter.h"
#include "../util/unique_ptr.h"

namespace berrydb {

class PoolTest : public ::testing::Test {
 protected:
  PoolTest()
      : vfs_(DefaultVfs()), data_file_deleter_(kFileName),
        log_file_deleter_(Store::LogFilePath(kFileName)) { }

  const std::string kFileName = "test_pool.berry";
  Vfs* vfs_;
  // Must precede UniquePtr members, because on Windows all file handles must be
  // closed before the files can be deleted.
  FileDeleter data_file_deleter_, log_file_deleter_;
};

TEST_F(PoolTest, CreateOptions) {
  PoolOptions options;
  options.page_shift = 12;
  options.page_pool_size = 42;

  UniquePtr<Pool> pool(Pool::Create(options));
  EXPECT_EQ(4096U, pool->page_size());
  EXPECT_EQ(42U, pool->page_pool_size());
}

TEST_F(PoolTest, ReleaseClosesStore) {
  PoolOptions pool_options;
  pool_options.page_shift = 12;
  pool_options.page_pool_size = 16;
  UniquePtr<Pool> pool(Pool::Create(pool_options));

  Store* raw_store = nullptr;
  StoreOptions options;
  options.create_if_missing = true;
  options.error_if_exists = false;
  ASSERT_EQ(Status::kSuccess, pool->OpenStore(kFileName, options, &raw_store));
  UniquePtr<Store> store(raw_store);

  EXPECT_FALSE(store->IsClosed());

  pool.reset();
  EXPECT_TRUE(store->IsClosed());
}

}  // namespace berrydb
