// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This is a subset of std::string_view from C++17. When compiler support
// becomes sufficiently advanced, this file will be reduced to a
// "using std::string_view" statement.

#ifndef BERRYDB_PLATFORM_STRING_VIEW_H_
#define BERRYDB_PLATFORM_STRING_VIEW_H_

#if BERRYDB_PLATFORM_HAVE_STD_STRING_VIEW

#include <string_view>

namespace berrydb {

using std::string_view;

}  // namespace berrydb

#else  // BERRYDB_PLATFORM_HAVE_STD_STRING_VIEW

#include <algorithm>
#include <cstddef>
#include <string>

#include "./dcheck.h"

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

  static const size_type npos = -1;

  constexpr inline string_view() noexcept : data_(nullptr), size_(0) { }
  constexpr inline string_view(const string_view&) noexcept = default;
  string_view& operator =(const string_view&) noexcept = default;
  // constexpr in C++14
  inline string_view(const_pointer data, size_type size) noexcept
      : data_(data), size_(size) {
    DCHECK(size == 0 || data != nullptr);
  }
  // constexpr in C++14
  inline string_view(const_pointer data) noexcept
      : data_(data), size_(traits_type::length(data)) { }

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

  // constexpr in C++14
  inline void remove_prefix(size_type n) noexcept {
    DCHECK_LE(n, size_);
    data_ += n;
    size_ -= n;
  }

  // constexpr in C++14
  inline void remove_suffix(size_type n) noexcept {
    DCHECK_LE(n, size_);
    size_ -= n;
  }

  // constexpr in C++14
  inline string_view substr(size_type pos = 0, size_type n = npos) const {
    DCHECK_LE(pos, size_);
    return string_view(data_ + pos, std::min(n, size_ - pos));
  }

  // constexpr in C++14
  inline int compare(const string_view& other) const noexcept {
    size_type common_size = std::min(size_, other.size_);
    int result = traits_type::compare(data_, other.data_, common_size);
    if (result == 0 && size_ != other.size_)
      result = (size_ < other.size_) ? -1 : 1;
    return result;
  }
 private:
  const_pointer data_;
  size_type size_;
};

// constexpr in C++14
inline bool operator ==(const string_view& l, const string_view& r) noexcept {
  return l.size() == r.size() && l.compare(r) == 0;
}
inline bool operator !=(const string_view& l, const string_view& r) noexcept {
  return l.size() != r.size() || l.compare(r) != 0;
}
inline bool operator <(const string_view& l, const string_view& r) noexcept {
  return l.compare(r) < 0;
}
inline bool operator <=(const string_view& l, const string_view& r) noexcept {
  return l.compare(r) <= 0;
}
inline bool operator >(const string_view& l, const string_view& r) noexcept {
  return l.compare(r) > 0;
}
inline bool operator >=(const string_view& l, const string_view& r) noexcept {
  return l.compare(r) >= 0;
}

}  // namespace berrydb

#endif  // BERRYDB_PLATFORM_HAVE_STD_STRING_VIEW

#endif  // BERRYDB_PLATFORM_STRING_VIEW_H_
