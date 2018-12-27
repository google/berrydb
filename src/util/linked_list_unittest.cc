// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "./linked_list.h"

#include <vector>
#include <utility>

#include "gtest/gtest.h"

namespace berrydb {

class TestListEmbedder {
 private:
  friend class LinkedListBridge<TestListEmbedder>;
  LinkedList<TestListEmbedder>::Node linked_list_node_;
};
using LinkedListWithStandardBridge = LinkedList<TestListEmbedder>;

class TestCustomListEmbedder {
 private:
  friend class TestCustomLinkedListBridge;
  LinkedListNode<TestCustomListEmbedder> custom_list_node_;
};
class TestCustomLinkedListBridge {
 public:
  using Embedder = TestCustomListEmbedder;
  using Node = LinkedListNode<Embedder>;

  static inline Node* NodeForHost(Embedder* host) noexcept {
    return &host->custom_list_node_;
  }
  static inline Embedder* HostForNode(Node* node) noexcept {
    static_assert(
        std::is_standard_layout<Embedder>::value,
        "Linked list embedders must be standard layout types");

    Embedder* host = reinterpret_cast<Embedder*>(
        reinterpret_cast<char*>(node) -
        offsetof(Embedder, custom_list_node_));
    BERRYDB_ASSUME_EQ(node, &host->custom_list_node_);
    return host;
  }
};
using LinkedListWithCustomBridge =
    LinkedList<TestCustomListEmbedder, TestCustomLinkedListBridge>;

static_assert(
    std::is_standard_layout<LinkedListWithStandardBridge::Node>::value,
    "LinkedListNode should be standard layout");
static_assert(
    std::is_standard_layout<LinkedListWithStandardBridge>::value,
    "LinkedList should be standard layout");

static_assert(
    std::is_standard_layout<LinkedListWithCustomBridge::Node>::value,
    "LinkedListNode should be standard layout");
static_assert(
    std::is_standard_layout<LinkedListWithCustomBridge>::value,
    "LinkedList should be standard layout");

template <typename ListType>
class LinkedListTest : public ::testing::Test {
 protected:
  ListType list_;
};

using LinkedListTestTypes =
    ::testing::Types<LinkedListWithStandardBridge, LinkedListWithCustomBridge>;
TYPED_TEST_CASE(LinkedListTest, LinkedListTestTypes);

TYPED_TEST(LinkedListTest, EmptyConstructor) {
  EXPECT_EQ(0U, this->list_.size());
  EXPECT_TRUE(this->list_.empty());

  EXPECT_EQ(this->list_.end(), this->list_.begin());
}

TYPED_TEST(LinkedListTest, Iterator) {
  typename TypeParam::EmbedderType host;
  this->list_.push_front(&host);

  EXPECT_EQ(this->list_.begin(), this->list_.begin());
  EXPECT_NE(this->list_.begin(), this->list_.end());

  typename TypeParam::iterator it = this->list_.begin();
  EXPECT_EQ(this->list_.begin(), it);

  typename TypeParam::iterator copy_it(it);
  EXPECT_EQ(this->list_.begin(), copy_it);

  typename TypeParam::iterator move_it(std::move(copy_it));
  EXPECT_EQ(this->list_.begin(), move_it);

  typename TypeParam::iterator copy_assign_it = this->list_.end();
  copy_assign_it = this->list_.begin();
  EXPECT_EQ(this->list_.begin(), copy_assign_it);

  typename TypeParam::iterator move_assign_it = this->list_.end();
  move_assign_it = std::move(copy_assign_it);
  EXPECT_EQ(this->list_.begin(), move_assign_it);

  typename TypeParam::iterator pre_increment_it = this->list_.begin();
  EXPECT_EQ(this->list_.end(), ++pre_increment_it);
  EXPECT_EQ(this->list_.end(), pre_increment_it);

  typename TypeParam::iterator pre_decrement_it = this->list_.end();
  EXPECT_EQ(this->list_.begin(), --pre_decrement_it);
  EXPECT_EQ(this->list_.begin(), pre_decrement_it);

  typename TypeParam::iterator post_increment_it = this->list_.begin();
  EXPECT_EQ(this->list_.begin(), post_increment_it++);
  EXPECT_EQ(this->list_.end(), post_increment_it);

  typename TypeParam::iterator post_decrement_it = this->list_.end();
  EXPECT_EQ(this->list_.end(), post_decrement_it--);
  EXPECT_EQ(this->list_.begin(), post_decrement_it);

  EXPECT_EQ(&host, *this->list_.begin());
}

TYPED_TEST(LinkedListTest, PushPopBack) {
  typename TypeParam::EmbedderType host1;
  this->list_.push_back(&host1);
  EXPECT_EQ(1U, this->list_.size());
  EXPECT_FALSE(this->list_.empty());

  EXPECT_EQ(&host1, *this->list_.begin());
  EXPECT_EQ(this->list_.end(), ++this->list_.begin());
  EXPECT_EQ(&host1, *(--this->list_.end()));

  typename TypeParam::EmbedderType host2;
  this->list_.push_back(&host2);
  EXPECT_EQ(2U, this->list_.size());
  EXPECT_FALSE(this->list_.empty());

  EXPECT_EQ(&host1, *this->list_.begin());
  EXPECT_EQ(&host2, *(++this->list_.begin()));
  EXPECT_EQ(this->list_.end(), ++(++this->list_.begin()));
  EXPECT_EQ(&host2, *(--this->list_.end()));
  EXPECT_EQ(&host1, *(--(--this->list_.end())));

  this->list_.pop_back();
  EXPECT_EQ(1U, this->list_.size());
  EXPECT_FALSE(this->list_.empty());

  EXPECT_EQ(&host1, *this->list_.begin());
  EXPECT_EQ(this->list_.end(), ++this->list_.begin());
  EXPECT_EQ(&host1, *(--this->list_.end()));

  this->list_.pop_back();
  EXPECT_EQ(0U, this->list_.size());
  EXPECT_TRUE(this->list_.empty());

  EXPECT_EQ(this->list_.end(), this->list_.begin());
}

TYPED_TEST(LinkedListTest, PushPopFront) {
  typename TypeParam::EmbedderType host1;
  this->list_.push_front(&host1);
  EXPECT_EQ(1U, this->list_.size());
  EXPECT_FALSE(this->list_.empty());

  EXPECT_EQ(&host1, *this->list_.begin());
  EXPECT_EQ(this->list_.end(), ++this->list_.begin());
  EXPECT_EQ(&host1, *(--this->list_.end()));

  typename TypeParam::EmbedderType host2;
  this->list_.push_front(&host2);
  EXPECT_EQ(2U, this->list_.size());
  EXPECT_FALSE(this->list_.empty());

  EXPECT_EQ(&host2, *this->list_.begin());
  EXPECT_EQ(&host1, *(++this->list_.begin()));
  EXPECT_EQ(this->list_.end(), ++(++this->list_.begin()));
  EXPECT_EQ(&host1, *(--this->list_.end()));
  EXPECT_EQ(&host2, *(--(--this->list_.end())));

  this->list_.pop_front();
  EXPECT_EQ(1U, this->list_.size());
  EXPECT_FALSE(this->list_.empty());

  EXPECT_EQ(&host1, *this->list_.begin());
  EXPECT_EQ(this->list_.end(), ++this->list_.begin());
  EXPECT_EQ(&host1, *(--this->list_.end()));

  this->list_.pop_front();
  EXPECT_EQ(0U, this->list_.size());
  EXPECT_TRUE(this->list_.empty());

  EXPECT_EQ(this->list_.end(), this->list_.begin());
}

TYPED_TEST(LinkedListTest, MoveConstructor) {
  TypeParam empty(std::move(this->list_));
  EXPECT_EQ(0U, empty.size());
  EXPECT_TRUE(empty.empty());
  EXPECT_EQ(empty.end(), empty.begin());

  typename TypeParam::EmbedderType host1;
  this->list_.push_back(&host1);
  TypeParam one{std::move(this->list_)};
  EXPECT_EQ(0U, this->list_.size());
  EXPECT_TRUE(this->list_.empty());
  EXPECT_EQ(this->list_.end(), this->list_.begin());

  EXPECT_EQ(1U, one.size());
  EXPECT_FALSE(one.empty());
  EXPECT_EQ(&host1, *one.begin());
  EXPECT_EQ(one.end(), ++one.begin());
  EXPECT_EQ(&host1, *(--one.end()));

  one.pop_back();
  EXPECT_EQ(0U, one.size());
  EXPECT_TRUE(one.empty());
  EXPECT_EQ(one.end(), one.begin());

  typename TypeParam::EmbedderType host2;
  this->list_.push_back(&host1);
  this->list_.push_back(&host2);
  TypeParam two{std::move(this->list_)};
  EXPECT_EQ(0U, this->list_.size());
  EXPECT_TRUE(this->list_.empty());
  EXPECT_EQ(this->list_.end(), this->list_.begin());

  EXPECT_EQ(2U, two.size());
  EXPECT_FALSE(two.empty());
  EXPECT_EQ(&host1, *two.begin());
  EXPECT_EQ(&host2, *(++two.begin()));
  EXPECT_EQ(two.end(), ++(++two.begin()));
  EXPECT_EQ(&host2, *(--two.end()));
  EXPECT_EQ(&host1, *(--(--two.end())));

  two.pop_back();
  EXPECT_EQ(1U, two.size());
  EXPECT_FALSE(two.empty());
  EXPECT_EQ(&host1, *two.begin());
  EXPECT_EQ(two.end(), ++two.begin());
  EXPECT_EQ(&host1, *(--two.end()));

  two.pop_back();
  EXPECT_EQ(0U, two.size());
  EXPECT_TRUE(two.empty());
  EXPECT_EQ(two.end(), two.begin());
}

TYPED_TEST(LinkedListTest, FrontBack) {
  typename TypeParam::EmbedderType host1, host2, host3;
  this->list_.push_back(&host1);
  this->list_.push_back(&host2);
  this->list_.push_back(&host3);

  EXPECT_EQ(&host1, this->list_.front());
  EXPECT_EQ(&host3, this->list_.back());
}

TYPED_TEST(LinkedListTest, Erase) {
  typename TypeParam::EmbedderType host1, host2, host3;
  this->list_.push_back(&host1);
  this->list_.push_back(&host2);
  this->list_.push_back(&host3);

  this->list_.erase(&host2);
  EXPECT_EQ(2U, this->list_.size());
  EXPECT_FALSE(this->list_.empty());

  EXPECT_EQ(&host1, *this->list_.begin());
  EXPECT_EQ(&host3, *(++this->list_.begin()));
  EXPECT_EQ(this->list_.end(), ++(++this->list_.begin()));
  EXPECT_EQ(&host3, *(--this->list_.end()));
  EXPECT_EQ(&host1, *(--(--this->list_.end())));
}

TYPED_TEST(LinkedListTest, RangedForLoop) {
  typename TypeParam::EmbedderType host1, host2, host3;
  this->list_.push_back(&host1);
  this->list_.push_back(&host2);
  this->list_.push_back(&host3);

  std::vector<typename TypeParam::EmbedderType*> values;
  for (typename TypeParam::EmbedderType* host : this->list_)
    values.push_back(host);

  EXPECT_EQ(3U, values.size());

  EXPECT_EQ(&host1, values[0]);
  EXPECT_EQ(&host2, values[1]);
  EXPECT_EQ(&host3, values[2]);
}

}  // namespace berrydb
