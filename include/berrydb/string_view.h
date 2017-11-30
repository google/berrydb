// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This is a subset of std::string_view from C++17. When compiler support
// becomes sufficiently advanced, this file will be reduced to a
// "using std::string_view" statement.

#ifndef BERRYDB_INCLUDE_BERRYDB_STRING_VIEW_H_
#define BERRYDB_INCLUDE_BERRYDB_STRING_VIEW_H_

#if __cplusplus > 201402L
#if __has_include(<string_view>)

#include <string_view>

namespace berrydb {

using std::string_view;

}  // namespace berrydb

#define BERRYDB_HAVE_STRING_VIEW_DEFINITION

#endif  // __has_include(<string_view>)
#endif  // __cplusplus > 201402L

#if !defined(BERRYDB_HAVE_STRING_VIEW_DEFINITION)

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <string>

namespace berrydb {

class string_view {
 public:
  typedef char value_type;

  typedef std::char_traits<value_type> traits_type;
  typedef const value_type* pointer;
  typedef const value_type* const_pointer;
  typedef const value_type& reference;
  typedef const value_type& const_reference;

  typedef const_pointer const_iterator;
  typedef const_iterator iterator;

  typedef size_t size_type;

  static constexpr const size_type npos = ~static_cast<size_t>(0);

  constexpr inline string_view() noexcept : data_(nullptr), size_(0) {}
  constexpr inline string_view(const string_view&) noexcept = default;
  string_view& operator=(const string_view&) noexcept = default;
  constexpr inline string_view(const_pointer data, size_type size) noexcept
      : data_(data), size_(size) {
    assert(size == 0 || data != nullptr);
  }
  // constexpr in C++17
  inline string_view(const_pointer data) noexcept
      : data_(data), size_(traits_type::length(data)) {}

  constexpr inline const_iterator cbegin() const noexcept { return data_; }
  constexpr inline const_iterator cend() const noexcept {
    return data_ + size_;
  }
  constexpr inline const_iterator begin() const noexcept { return cbegin(); }
  constexpr inline const_iterator end() const noexcept { return cend(); }

  constexpr inline size_type size() const noexcept { return size_; }
  constexpr inline size_type length() const noexcept { return size_; }
  constexpr inline bool empty() const noexcept { return size_ == 0; }

  constexpr inline const_reference operator[](size_type pos) const noexcept {
    return data_[pos];
  }

  constexpr inline const_pointer data() const noexcept { return data_; }

  constexpr inline void remove_prefix(size_type n) noexcept {
    assert(n <= size_);
    data_ += n;
    size_ -= n;
  }

  constexpr inline void remove_suffix(size_type n) noexcept {
    assert(n <= size_);
    size_ -= n;
  }

  size_type copy(char* to, size_type n, size_type pos = 0) {
    assert(pos <= size_);
    size_t copy_size = std::min(n, size_ - pos);
    traits_type::copy(to, data_ + pos, copy_size);
    return copy_size;
  }

  constexpr inline string_view substr(
      size_type pos = 0, size_type n = npos) const {
    assert(pos <= size_);
    return string_view(data_ + pos, std::min(n, size_ - pos));
  }

  // constexpr in C++17
  inline int compare(const string_view& other) const noexcept {
    size_type common_size = std::min(size_, other.size_);
    int result = traits_type::compare(data_, other.data_, common_size);
    if (result == 0 && size_ != other.size_)
      result = (size_ < other.size_) ? -1 : 1;
    return result;
  }

  // Hack to make up for the lack of a std::string constructor override.
  template <typename Allocator>
  inline constexpr
  operator std::basic_string<char, std::char_traits<char>, Allocator>() const {
    return std::basic_string<char, std::char_traits<char>, Allocator>(
        data_, size_, Allocator());
  }

 private:
  const_pointer data_;
  size_type size_;
};

constexpr inline bool operator==(
    const string_view& l, const string_view& r) noexcept {
  return l.size() == r.size() && l.compare(r) == 0;
}
inline bool operator!=(const string_view& l, const string_view& r) noexcept {
  return l.size() != r.size() || l.compare(r) != 0;
}
inline bool operator<(const string_view& l, const string_view& r) noexcept {
  return l.compare(r) < 0;
}
inline bool operator<=(const string_view& l, const string_view& r) noexcept {
  return l.compare(r) <= 0;
}
inline bool operator>(const string_view& l, const string_view& r) noexcept {
  return l.compare(r) > 0;
}
inline bool operator>=(const string_view& l, const string_view& r) noexcept {
  return l.compare(r) >= 0;
}

}  // namespace berrydb

#endif  // !defined(BERRYDB_HAVE_STRING_VIEW_DEFINITION)
#undef BERRYDB_HAVE_STRING_VIEW_DEFINITION

#endif  // BERRYDB_INCLUDE_BERRYDB_STRING_VIEW_H_
