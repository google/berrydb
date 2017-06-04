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
#include "../util/unique_ptr.h"

namespace berrydb {

class StoreTest : public ::testing::Test {
 protected:
  void SetUp() override {
    vfs_ = DefaultVfs();
    vfs_->DeleteFile(kFileName);
    vfs_->DeleteFile(Store::LogFilePath(kFileName));

    PoolOptions options;
    options.page_shift = 12;
    options.page_pool_size = 42;
    pool_.reset(Pool::Create(options));
  }
  void TearDown() override {
    pool_.reset();  // Must happen before the file is deleted.
    vfs_->DeleteFile(kFileName);
    vfs_->DeleteFile(Store::LogFilePath(kFileName));
  }

  const std::string kFileName = "test_store.berry";
  Vfs* vfs_;
  UniquePtr<Pool> pool_;
};

TEST_F(StoreTest, CreateOptions) {
  // This test case doesn't use UniquePtr because the wrapping code would make
  // the test case double in size.
  Store* store = nullptr;
  StoreOptions options;

  // Setup guarantees that the store does not exist.
  options.create_if_missing = false;
  ASSERT_NE(Status::kSuccess, pool_->OpenStore(kFileName, options, &store));
  EXPECT_EQ(nullptr, store);

  options.create_if_missing = true;
  options.error_if_exists = true;
  store = nullptr;
  ASSERT_EQ(Status::kSuccess, pool_->OpenStore(kFileName, options, &store));
  ASSERT_NE(nullptr, store);
  store->Release();

  // The ASSERT above guarantees that the store was created.
  store = nullptr;
  ASSERT_NE(Status::kSuccess, pool_->OpenStore(kFileName, options, &store));
  EXPECT_EQ(nullptr, store);

  options.error_if_exists = false;
  store = nullptr;
  ASSERT_EQ(Status::kSuccess, pool_->OpenStore(kFileName, options, &store));
  ASSERT_NE(nullptr, store);
  store->Release();

  options.create_if_missing = false;
  store = nullptr;
  ASSERT_EQ(Status::kSuccess, pool_->OpenStore(kFileName, options, &store));
  ASSERT_NE(nullptr, store);
  store->Release();
}

TEST_F(StoreTest, CloseAbortsTransaction) {
  Store* raw_store = nullptr;
  StoreOptions options;
  ASSERT_EQ(Status::kSuccess, pool_->OpenStore(kFileName, options, &raw_store));
  UniquePtr<Store> store(raw_store);

  UniquePtr<Transaction> transaction(store->CreateTransaction());
  EXPECT_FALSE(transaction->IsCommitted());
  EXPECT_FALSE(transaction->IsRolledBack());
  EXPECT_FALSE(transaction->IsClosed());

  store.reset();
  EXPECT_FALSE(transaction->IsCommitted());
  EXPECT_TRUE(transaction->IsRolledBack());
  EXPECT_TRUE(transaction->IsClosed());
}

}  // namespace berrydb
