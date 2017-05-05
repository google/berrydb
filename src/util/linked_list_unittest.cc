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
  friend class LinkedList<TestListEmbedder>;
  LinkedList<TestListEmbedder>::Node linked_list_node_;
};

class LinkedListTest : public ::testing::Test {
 protected:
  LinkedList<TestListEmbedder> list_;
};

TEST_F(LinkedListTest, EmptyConstructor) {
  EXPECT_EQ(0U, list_.size());
  EXPECT_TRUE(list_.empty());

  EXPECT_EQ(list_.end(), list_.begin());
}

TEST_F(LinkedListTest, Iterator) {
  TestListEmbedder host;
  list_.push_front(&host);

  EXPECT_EQ(list_.begin(), list_.begin());
  EXPECT_NE(list_.begin(), list_.end());

  LinkedList<TestListEmbedder>::iterator it = list_.begin();
  EXPECT_EQ(list_.begin(), it);

  LinkedList<TestListEmbedder>::iterator copy_it(it);
  EXPECT_EQ(list_.begin(), copy_it);

  LinkedList<TestListEmbedder>::iterator move_it(std::move(copy_it));
  EXPECT_EQ(list_.begin(), move_it);

  LinkedList<TestListEmbedder>::iterator copy_assign_it = list_.end();
  copy_assign_it = list_.begin();
  EXPECT_EQ(list_.begin(), copy_assign_it);

  LinkedList<TestListEmbedder>::iterator move_assign_it = list_.end();
  move_assign_it = std::move(copy_assign_it);
  EXPECT_EQ(list_.begin(), move_assign_it);

  LinkedList<TestListEmbedder>::iterator pre_increment_it = list_.begin();
  EXPECT_EQ(list_.end(), ++pre_increment_it);
  EXPECT_EQ(list_.end(), pre_increment_it);

  LinkedList<TestListEmbedder>::iterator pre_decrement_it = list_.end();
  EXPECT_EQ(list_.begin(), --pre_decrement_it);
  EXPECT_EQ(list_.begin(), pre_decrement_it);

  LinkedList<TestListEmbedder>::iterator post_increment_it = list_.begin();
  EXPECT_EQ(list_.begin(), post_increment_it++);
  EXPECT_EQ(list_.end(), post_increment_it);

  LinkedList<TestListEmbedder>::iterator post_decrement_it = list_.end();
  EXPECT_EQ(list_.end(), post_decrement_it--);
  EXPECT_EQ(list_.begin(), post_decrement_it);

  EXPECT_EQ(&host, *list_.begin());
}

TEST_F(LinkedListTest, PushPopBack) {
  TestListEmbedder host1;
  list_.push_back(&host1);
  EXPECT_EQ(1U, list_.size());
  EXPECT_FALSE(list_.empty());

  EXPECT_EQ(&host1, *list_.begin());
  EXPECT_EQ(list_.end(), ++list_.begin());
  EXPECT_EQ(&host1, *(--list_.end()));

  TestListEmbedder host2;
  list_.push_back(&host2);
  EXPECT_EQ(2U, list_.size());
  EXPECT_FALSE(list_.empty());

  EXPECT_EQ(&host1, *list_.begin());
  EXPECT_EQ(&host2, *(++list_.begin()));
  EXPECT_EQ(list_.end(), ++(++list_.begin()));
  EXPECT_EQ(&host2, *(--list_.end()));
  EXPECT_EQ(&host1, *(--(--list_.end())));

  list_.pop_back();
  EXPECT_EQ(1U, list_.size());
  EXPECT_FALSE(list_.empty());

  EXPECT_EQ(&host1, *list_.begin());
  EXPECT_EQ(list_.end(), ++list_.begin());
  EXPECT_EQ(&host1, *(--list_.end()));

  list_.pop_back();
  EXPECT_EQ(0U, list_.size());
  EXPECT_TRUE(list_.empty());

  EXPECT_EQ(list_.end(), list_.begin());
}

TEST_F(LinkedListTest, PushPopFront) {
  TestListEmbedder host1;
  list_.push_front(&host1);
  EXPECT_EQ(1U, list_.size());
  EXPECT_FALSE(list_.empty());

  EXPECT_EQ(&host1, *list_.begin());
  EXPECT_EQ(list_.end(), ++list_.begin());
  EXPECT_EQ(&host1, *(--list_.end()));

  TestListEmbedder host2;
  list_.push_front(&host2);
  EXPECT_EQ(2U, list_.size());
  EXPECT_FALSE(list_.empty());

  EXPECT_EQ(&host2, *list_.begin());
  EXPECT_EQ(&host1, *(++list_.begin()));
  EXPECT_EQ(list_.end(), ++(++list_.begin()));
  EXPECT_EQ(&host1, *(--list_.end()));
  EXPECT_EQ(&host2, *(--(--list_.end())));

  list_.pop_front();
  EXPECT_EQ(1U, list_.size());
  EXPECT_FALSE(list_.empty());

  EXPECT_EQ(&host1, *list_.begin());
  EXPECT_EQ(list_.end(), ++list_.begin());
  EXPECT_EQ(&host1, *(--list_.end()));

  list_.pop_front();
  EXPECT_EQ(0U, list_.size());
  EXPECT_TRUE(list_.empty());

  EXPECT_EQ(list_.end(), list_.begin());
}

TEST_F(LinkedListTest, MoveConstructor) {
  LinkedList<TestListEmbedder> empty(std::move(list_));
  EXPECT_EQ(0U, empty.size());
  EXPECT_TRUE(empty.empty());
  EXPECT_EQ(empty.end(), empty.begin());

  TestListEmbedder host1;
  list_.push_back(&host1);
  LinkedList<TestListEmbedder> one{std::move(list_)};
  EXPECT_EQ(0U, list_.size());
  EXPECT_TRUE(list_.empty());
  EXPECT_EQ(list_.end(), list_.begin());

  EXPECT_EQ(1U, one.size());
  EXPECT_FALSE(one.empty());
  EXPECT_EQ(&host1, *one.begin());
  EXPECT_EQ(one.end(), ++one.begin());
  EXPECT_EQ(&host1, *(--one.end()));

  one.pop_back();
  EXPECT_EQ(0U, one.size());
  EXPECT_TRUE(one.empty());
  EXPECT_EQ(one.end(), one.begin());

  TestListEmbedder host2;
  list_.push_back(&host1);
  list_.push_back(&host2);
  LinkedList<TestListEmbedder> two{std::move(list_)};
  EXPECT_EQ(0U, list_.size());
  EXPECT_TRUE(list_.empty());
  EXPECT_EQ(list_.end(), list_.begin());

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

TEST_F(LinkedListTest, FrontBack) {
  TestListEmbedder host1, host2, host3;
  list_.push_back(&host1);
  list_.push_back(&host2);
  list_.push_back(&host3);

  EXPECT_EQ(&host1, list_.front());
  EXPECT_EQ(&host3, list_.back());
}

TEST_F(LinkedListTest, Erase) {
  TestListEmbedder host1, host2, host3;
  list_.push_back(&host1);
  list_.push_back(&host2);
  list_.push_back(&host3);

  list_.erase(&host2);
  EXPECT_EQ(2U, list_.size());
  EXPECT_FALSE(list_.empty());

  EXPECT_EQ(&host1, *list_.begin());
  EXPECT_EQ(&host3, *(++list_.begin()));
  EXPECT_EQ(list_.end(), ++(++list_.begin()));
  EXPECT_EQ(&host3, *(--list_.end()));
  EXPECT_EQ(&host1, *(--(--list_.end())));
}

TEST_F(LinkedListTest, RangedForLoop) {
  TestListEmbedder host1, host2, host3;
  list_.push_back(&host1);
  list_.push_back(&host2);
  list_.push_back(&host3);

  std::vector<TestListEmbedder*> values;
  for (TestListEmbedder* host : list_)
    values.push_back(host);

  EXPECT_EQ(3U, values.size());

  EXPECT_EQ(&host1, values[0]);
  EXPECT_EQ(&host2, values[1]);
  EXPECT_EQ(&host3, values[2]);
}

}  // namespace berrydb
