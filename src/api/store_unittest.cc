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
#include "../test/file_deleter.h"
#include "../util/unique_ptr.h"

namespace berrydb {

class StoreTest : public ::testing::Test {
 protected:
  StoreTest()
      : vfs_(DefaultVfs()), data_file_deleter_(kFileName),
        log_file_deleter_(Store::LogFilePath(kFileName)) { }

  void SetUp() override {
    PoolOptions options;
    options.page_shift = 12;
    options.page_pool_size = 42;
    pool_.reset(Pool::Create(options));
  }

  const std::string kFileName = "test_store.berry";
  Vfs* vfs_;
  // Must precede UniquePtr members, because on Windows all file handles must be
  // closed before the files can be deleted.
  FileDeleter data_file_deleter_, log_file_deleter_;

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
  EXPECT_EQ(Status::kSuccess, store->Close());
  store->Release();

  // The ASSERT above guarantees that the store was created.
  store = nullptr;
  ASSERT_NE(Status::kSuccess, pool_->OpenStore(kFileName, options, &store));
  EXPECT_EQ(nullptr, store);

  options.error_if_exists = false;
  store = nullptr;
  ASSERT_EQ(Status::kSuccess, pool_->OpenStore(kFileName, options, &store));
  ASSERT_NE(nullptr, store);
  EXPECT_EQ(Status::kSuccess, store->Close());
  store->Release();

  options.create_if_missing = false;
  store = nullptr;
  ASSERT_EQ(Status::kSuccess, pool_->OpenStore(kFileName, options, &store));
  ASSERT_NE(nullptr, store);
  EXPECT_EQ(Status::kSuccess, store->Close());
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
