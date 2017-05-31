// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "berrydb/store.h"

#include <string>

#include "gtest/gtest.h"

#include "berrydb/options.h"
#include "berrydb/pool.h"
#include "berrydb/status.h"
#include "berrydb/transaction.h"
#include "berrydb/vfs.h"

namespace berrydb {

class StoreTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    vfs_ = DefaultVfs();
    vfs_->DeleteFile(kFileName);
    vfs_->DeleteFile(Store::LogFilePath(kFileName));

    PoolOptions options;
    options.page_shift = 12;
    options.page_pool_size = 42;
    pool_ = Pool::Create(options);
  }
  virtual void TearDown() {
    pool_->Release();
    vfs_->DeleteFile(kFileName);
    vfs_->DeleteFile(Store::LogFilePath(kFileName));
  }

  const std::string kFileName = "test_store.berry";
  Vfs* vfs_;
  Pool* pool_;
};

TEST_F(StoreTest, CreateOptions) {
  Store* store = nullptr;
  StoreOptions options;

  // Setup guarantees that the store does not exist.
  options.create_if_missing = false;
  ASSERT_NE(Status::kSuccess, pool_->OpenStore(kFileName, options, &store));
  EXPECT_EQ(nullptr, store);

  options.create_if_missing = true;
  options.error_if_exists = true;
  ASSERT_EQ(Status::kSuccess, pool_->OpenStore(kFileName, options, &store));
  store->Release();

  // The ASSERT above guarantees that the store was created.
  store = nullptr;
  ASSERT_NE(Status::kSuccess, pool_->OpenStore(kFileName, options, &store));
  EXPECT_EQ(nullptr, store);

  options.error_if_exists = false;
  ASSERT_EQ(Status::kSuccess, pool_->OpenStore(kFileName, options, &store));
  store->Release();

  options.create_if_missing = false;
  ASSERT_EQ(Status::kSuccess, pool_->OpenStore(kFileName, options, &store));
  store->Release();
}

TEST_F(StoreTest, CloseAbortsTransaction) {
  Store* store = nullptr;
  StoreOptions options;
  ASSERT_EQ(Status::kSuccess, pool_->OpenStore(kFileName, options, &store));

  Transaction* transaction = store->CreateTransaction();
  EXPECT_FALSE(transaction->IsCommitted());
  EXPECT_FALSE(transaction->IsRolledBack());
  EXPECT_FALSE(transaction->IsClosed());

  store->Release();
  EXPECT_FALSE(transaction->IsCommitted());
  EXPECT_TRUE(transaction->IsRolledBack());
  EXPECT_TRUE(transaction->IsClosed());
}

}  // namespace berrydb
