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

namespace berrydb {

class PoolTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    vfs_ = DefaultVfs();
    vfs_->DeleteFile(kFileName);
  }

  virtual void TearDown() {
    vfs_->DeleteFile(kFileName);
  }

  const std::string kFileName = "test_pool.berry";
  Vfs* vfs_;
};

TEST_F(PoolTest, CreateOptions) {
  PoolOptions options;
  options.page_shift = 12;
  options.page_pool_size = 42;

  Pool* pool = Pool::Create(options);
  EXPECT_EQ(4096U, pool->page_size());
  EXPECT_EQ(42U, pool->page_pool_size());

  pool->Release();
}

TEST_F(PoolTest, ReleaseClosesStore) {
  PoolOptions pool_options;
  pool_options.page_shift = 12;
  pool_options.page_pool_size = 16;
  Pool* pool = Pool::Create(pool_options);

  Store* store = nullptr;
  StoreOptions options;
  options.create_if_missing = true;
  options.error_if_exists = false;
  ASSERT_EQ(Status::kSuccess, pool->OpenStore(kFileName, options, &store));

  EXPECT_FALSE(store->IsClosed());

  pool->Release();
  EXPECT_TRUE(store->IsClosed());
}

}  // namespace berrydb
