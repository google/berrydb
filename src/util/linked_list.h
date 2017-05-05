// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_UTIL_LINKED_LIST_H_
#define BERRYDB_UTIL_LINKED_LIST_H_

#include "berrydb/platform.h"

#include <utility>

namespace berrydb {

/**
 * A doubly linked list with embeddable nodes.
 *
 * This custom solution reduces dynamic memory allocations by embedding the
 * list's noeds into the host data structure. If memory isn't an issue,
 * std::list<Host*> should be preferred.
 *
 * Embedder code should look as follows:
 *
 *     class Embedder {
 *      private:
 *       LinkedList<Embedder>::Node linked_list_node_;
 *       friend class LinkedList<Embedder>;
 *     }
 *
 * The embedder class must be a standard layout type (can be checked using
 * std::is_standard_layout). Linked list operations will be a bit faster when
 * the LinkedList::Node is the first member of the embedder class.
 *
 * This class implements the std::list subset used in the project. The subset
 * may grow over time. However, the following will never be implemented, as a
 * consequence of having embedded nodes.
 * 1) copy constructor and assignment - impossible, because each embedded node
 *        can be in at most one list at a time
 * 2) emplace_* - doesn't really make sense, given that the embedders contain
 *        the nodes, not the other way around
 */
template<typename Embedder>
class LinkedList {
 public:
  using value_type = Embedder*;
  using size_type = size_t;
  using reference = value_type&;
  using const_reference = const value_type&;
  class iterator;

  inline LinkedList() noexcept : sentinel_(this), size_(0) { }
  inline LinkedList(LinkedList&& other) noexcept
      : sentinel_(this, std::move(other.sentinel_)), size_(other.size_) {
    other.size_ = 0;
  }
  inline ~LinkedList() noexcept = default;

  LinkedList(const LinkedList&) = delete;
  LinkedList& operator=(const LinkedList&) = delete;

  inline bool empty() const noexcept { return size_ == 0; }
  inline size_t size() const noexcept { return size_; }

  inline iterator begin() noexcept {
    return iterator{sentinel_.next()};
  }
  inline iterator end() noexcept {
    return iterator{&sentinel_};
  }

  inline value_type front() noexcept { return *begin(); }
  inline value_type back() noexcept { return *(--end()); }

  inline void insert(iterator pos, value_type value) noexcept {
    DCHECK(value != nullptr);

    Node* node = LinkedList::NodeForHost(value);
    DCHECK_EQ(value, LinkedList::HostForNode(node));

    node->InsertBefore(pos.node_);
    ++size_;
  }

  inline void erase(iterator pos) noexcept {
    Node* node = pos.node_;
    DCHECK(node != nullptr);

#if DCHECK_IS_ON()
    DCHECK(!node->is_sentinel_);
    DCHECK_EQ(this, node->list_);
#endif  // DCHECK_IS_ON()

    node->Remove();
    DCHECK(size_ > 0);
    --size_;
  }

  inline void erase(value_type value) noexcept {
    Node* node = LinkedList::NodeForHost(value);

#if DCHECK_IS_ON()
    DCHECK_EQ(this, node->list_);
#endif  // DCHECK_IS_ON()

    node->Remove();
    DCHECK(size_ > 0);
    --size_;
  }

  inline void push_front(value_type value) noexcept { insert(begin(), value); }
  inline void push_back(value_type value) noexcept { insert(end(), value); }
  inline void pop_front() noexcept { erase(begin()); }
  inline void pop_back() noexcept { erase(--end()); }

  /** A node in the list. */
  class Node {
   public:
    /** Constructor for non-sentinel nodes. */
    inline Node()
#if DCHECK_IS_ON()
        : next_(nullptr), prev_(nullptr), list_(nullptr), is_sentinel_(false)
#endif  // DCHECK_IS_ON()
        { }

#if DCHECK_IS_ON()
    /** Only intended for use in DCHECKs. */
    inline LinkedList* list() const noexcept { return list_; }
#endif  // DCHECK_ISON()

   private:
    /** Constructor for sentinel nodes. */
    inline Node(LinkedList* list)
        : next_(this), prev_(this)
#if DCHECK_IS_ON()
        , list_(list), is_sentinel_(true)
#endif  // DCHECK_IS_ON()
        {
      DCHECK(list != nullptr);
      UNUSED(list);
    }

    /** Used by the list move-constructor. */
    inline Node(LinkedList* list, Node&& other)
#if DCHECK_IS_ON()
        : list_(list), is_sentinel_(true)
#endif  // DCHECK_IS_ON()
        {
#if DCHECK_IS_ON()
      DCHECK(other.is_sentinel_);
#endif  // DCHECK_ISON()

      if (other.next() == &other) {
        // Empty list.
        next_ = prev_ = this;
        return;
      }

      this->next_ = other.next_;
      other.next_ = &other;
      this->prev_ = other.prev_;
      other.prev_ = &other;
      this->next_->prev_ = this;
      this->prev_->next_ = this;

      // This is actually O(list size) when DCHECK is enabled. This is only
      // mildly unfortunate, because list-moves are generally used when a list's
      // items are destroyed.
#if DCHECK_IS_ON()
      for (Node* node = next_; node != this; node = node->next_) {
        DCHECK(!node->is_sentinel_);
        DCHECK_EQ(node->list_, other.list_);
        DCHECK(node->next_ != nullptr);
        DCHECK(node->prev_ != nullptr);

        node->list_ = list;
      }
#endif  // DCHECK_IS_ON()
    }

    friend class LinkedList;
    friend class iterator;

    inline Node* next() const noexcept {
#if DCHECK_IS_ON()
      DCHECK(list_ != nullptr);
#endif  // DCHECK_IS_ON()

      // Redundant with check above, might trigger if memory gets corrupted.
      DCHECK(next_ != nullptr);
      DCHECK(next_->prev_ == this);

      return next_;
    }
    inline Node* prev() const noexcept {
#if DCHECK_IS_ON()
      DCHECK(list_ != nullptr);
#endif  // DCHECK_IS_ON()

      // Redundant with check above, might trigger if memory gets corrupted.
      DCHECK(prev_ != nullptr);
      DCHECK(prev_->next_ == this);

      return prev_;
    }
    inline Embedder* host() noexcept { return LinkedList::HostForNode(this); }

    /** Inserts this node in a list, before a given node. */
    inline void InsertBefore(Node* next) noexcept {
#if DCHECK_IS_ON()
      DCHECK(!is_sentinel_);
      DCHECK(list_ == nullptr);  // The node cannot already be in a list.

      // Redundant with check above, might trigger if memory gets corrupted.
      DCHECK(next_ == nullptr);
      DCHECK(prev_ == nullptr);

      DCHECK(next->list_ != nullptr);  // The other node must be in a list.

      list_ = next->list_;
#endif  // DCHECK_IS_ON()

      // Redundant with check above, might trigger if memory gets corrupted.
      DCHECK(next->next_ != nullptr);
      DCHECK(next->prev_ != nullptr);

      this->prev_ = next->prev_;
      next->prev_->next_ = this;
      this->next_ = next;
      next->prev_ = this;
    }

    /** Removes this node from the list that it is in. */
    inline void Remove() noexcept {
#if DCHECK_IS_ON()
      DCHECK(!is_sentinel_);
      DCHECK(list_ != nullptr);  // The node must be in a list.

      list_ = nullptr;
#endif  // DCHECK_IS_ON()

      // Redundant with check above, might trigger if memory gets corrupted.
      DCHECK(next_ != nullptr);
      DCHECK(prev_ != nullptr);

      this->next_->prev_ = this->prev_;
      this->prev_->next_ = this->next_;
#if DCHECK_IS_ON()
      this->next_ = nullptr;
      this->prev_ = nullptr;
#endif  // DCHECK_IS_ON()
    }

    Node* next_;
    Node* prev_;

#if DCHECK_IS_ON()
    /** The list that the node is inserted into. */
    LinkedList* list_;
    const bool is_sentinel_;
#endif  // DCHECK_IS_ON()
  };

  /** BidirectionalIterator wrapper around a Node pointer. */
  class iterator {
   public:
    inline iterator(const iterator&) noexcept = default;
    inline iterator(iterator&&) noexcept = default;
    inline iterator& operator=(const iterator&) noexcept = default;
    inline iterator& operator=(iterator&&) noexcept = default;
    inline ~iterator() noexcept = default;

    inline bool operator ==(const iterator& other) const noexcept {
      return node_ == other.node_;
    }
    inline bool operator !=(const iterator& other) const noexcept {
      return node_ != other.node_;
    }

    inline iterator& operator++() noexcept {
#if DCHECK_IS_ON()
      DCHECK(!node_->is_sentinel_);  // Already at cend().
#endif  // DCHECK_IS_ON()
      node_ = node_->next();
      return *this;
    }
    inline iterator operator++(int) noexcept {
#if DCHECK_IS_ON()
      DCHECK(!node_->is_sentinel_);  // Already at cend().
#endif  // DCHECK_IS_ON()
      Node* old_node = node_;
      node_ = node_->next();
      return iterator(old_node);
    }
    inline iterator& operator--() noexcept {
#if DCHECK_IS_ON()
      DCHECK(!node_->prev()->is_sentinel_);  // Already at cbegin().
#endif  // DCHECK_IS_ON()
      node_ = node_->prev();
      return *this;
    }
    inline iterator operator--(int) noexcept {
#if DCHECK_IS_ON()
      DCHECK(!node_->prev()->is_sentinel_);  // Already at cbegin().
#endif  // DCHECK_IS_ON()
      Node* old_node = node_;
      node_ = node_->prev();
      return iterator(old_node);
    }

    inline value_type operator*() const noexcept { return node_->host(); }

   private:
    /** Constructor used by the list. */
    iterator(Node* node): node_(node) { DCHECK(node != nullptr); }

    friend class LinkedList;

    Node* node_;
  };

 private:
  /** Extracts the Node from an embedder object.
   *
   * This method is static so other LinkedList sub-classes can use it without
   * having to be friends of the embedder.
   */
  static inline Node* NodeForHost(Embedder* host) noexcept {
    return &host->linked_list_node_;
  }

  /** Converts a node pointer back to an embedder pointer.
   *
   * This method is static so other LinkedList sub-classes can use it without
   * having to be friends of the embedder.
   */
  static inline Embedder* HostForNode(Node* node) noexcept {
    static_assert(
        std::is_standard_layout<Embedder>::value,
        "Linked list embedders must be standard layout types");
#if DCHECK_IS_ON()
    DCHECK(!node->is_sentinel_);
#endif  // DCHECK_IS_ON()

    Embedder* host = reinterpret_cast<Embedder*>(
        reinterpret_cast<char*>(node) -
        offsetof(Embedder, linked_list_node_));
    DCHECK_EQ(node, &host->linked_list_node_);
    return host;
  }

  Node sentinel_;
  size_t size_;
};

}  // namespace berrydb

#endif  // BERRYDB_UTIL_LINKED_LIST_H_
