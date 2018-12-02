// Copyright 2018 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// C++14 implementation of a subset of std::span from C++ 2a.
//
// When compiler support becomes sufficiently advanced, this file will be
// reduced to a "using std::span".

#ifndef BERRYDB_INCLUDE_BERRYDB_SPAN_H_
#define BERRYDB_INCLUDE_BERRYDB_SPAN_H_

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <type_traits>

namespace berrydb {

// Equivalent to an alias of std::span from C++ 2a.
template <typename ElementType>
class span {
 public:
  using element_type = ElementType;
  using value_type = std::remove_cv_t<ElementType>;
  using pointer = ElementType*;
  using reference = ElementType&;

#if defined(_MSC_VER) && !defined(_LIBCPP_STD_VER)
  // Checked iterators are a Visual Studio C++ specific extension. They're
  // necessary to avoid warnings when using standard library methods that
  // operate on iterators, like std::copy.
  using iterator = stdext::checked_array_iterator<ElementType*>;

  // const_iterator should use checked_array_iterator<const ElementType*>.
  // However, checked_array_iterator<T*> is not convertible to
  // checked_array_iterator<const T*>, and this causes compilation errors in
  // googletest's DefaultPrintTo code, which uses something like:
  //   Container::const_iterator it = container.begin();
  //
  // This simplistic workaround is acceptable because this project has other
  // compilers on CI, which alias const_iterator to const ElementType*, so const
  // errors can't get through.
  using const_iterator = stdext::checked_array_iterator<ElementType*>;
#else
  using iterator = ElementType*;
  using const_iterator = const ElementType*;
#endif  // defined(_MSC_VER) && !defined(_LIBCPP_STD_VER)

  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  inline constexpr span() noexcept : data_(nullptr), size_(0) {}
  inline constexpr span(ElementType* data, std::size_t size) noexcept
      : data_(data), size_(size) {
    assert(!size_ || data_ != nullptr);
  }

  // The template is a workaround for disqualifying this constructor in the
  // lookup for span(pointer, 0).
  template <typename = void>
  inline constexpr span(ElementType* begin, ElementType* end)
      : data_(begin), size_(end - begin) {
    assert(begin <= end);
    assert(!size_ || data_ != nullptr);
  }

  // TODO(pwnall): Add container and array constructors, if/when necessary.

  inline constexpr span(const span& other) noexcept = default;
  inline constexpr span& operator=(const span& other) noexcept = default;

  // TODO(pwnall): Switch to std::is_convertible_v when migrating to C++17.
  template <
      typename OtherElementType,
      typename = std::enable_if_t<std::is_convertible<
          OtherElementType (*)[], ElementType (*)[]>::value>>
  inline constexpr span(const span<OtherElementType>& other)
      : data_(other.data()), size_(other.size()) {}

  template <std::size_t ArraySize>
  inline constexpr span(ElementType (&array)[ArraySize]) noexcept :
      data_(array), size_(ArraySize) {
    assert(!size_ || data_ != nullptr);
  }

  inline ~span() noexcept = default;

  inline constexpr span first(std::size_t count) const noexcept {
    assert(count <= size_);
    return span(data_, count);
  }
  inline constexpr span last(std::size_t count) const noexcept {
    assert(count <= size_);
    return span(data_ + (size_ - count), count);
  }
  inline constexpr span subspan(std::size_t pos,
                                std::size_t count = -1) const noexcept {
    // Value of std::dynamic_extent.
    std::size_t dynamic_extent = static_cast<std::size_t>(-1);
    assert(pos <= size_);
    assert(count == dynamic_extent || count <= size_ - pos);
    return span(data_ + pos, (count == dynamic_extent) ? size_ - pos : count);
  }

  inline constexpr std::size_t size() const noexcept { return size_; }
  inline constexpr std::size_t size_bytes() const noexcept {
    return size_ * sizeof(ElementType);
  }
  inline constexpr bool empty() const noexcept { return !size_; }

  inline constexpr ElementType& operator[](std::size_t index) const noexcept {
    assert(index < size_);
    return data_[index];
  }
  inline constexpr ElementType& operator()(std::size_t index) const noexcept {
    assert(index < size_);
    return data_[index];
  }
  inline constexpr ElementType* data() const noexcept { return data_; }

#if defined(_MSC_VER) && !defined(_LIBCPP_STD_VER)
  inline constexpr iterator begin() const noexcept {
    return stdext::checked_array_iterator<ElementType*>(data_, size_);
  }
  inline constexpr iterator end() const noexcept {
    return stdext::checked_array_iterator<ElementType*>(data_, size_, size_);
  }
  inline constexpr const_iterator cbegin() const noexcept {
    return stdext::checked_array_iterator<ElementType*>(data_, size_);
  }
  inline constexpr const_iterator cend() const noexcept {
    return stdext::checked_array_iterator<ElementType*>(data_, size_, size_);
  }
#else
  inline constexpr iterator begin() const noexcept { return data_; }
  inline constexpr iterator end() const noexcept { return data_ + size_; }
  inline constexpr const_iterator cbegin() const noexcept { return begin(); }
  inline constexpr const_iterator cend() const noexcept { return end(); }
#endif  // defined(_MSC_VER) && !defined(_LIBCPP_STD_VER)

  inline constexpr reverse_iterator rbegin() const noexcept {
    return reverse_iterator(end());
  }
  inline constexpr reverse_iterator rend() const noexcept {
    return reverse_iterator(begin());
  }
  inline constexpr const_reverse_iterator crbegin() const noexcept {
    return const_reverse_iterator(cend());
  }
  inline constexpr const_reverse_iterator crend() const noexcept {
    return const_reverse_iterator(cbegin());
  }

 private:
  ElementType* data_;
  std::size_t size_;
};

template <typename ElementType, typename OtherElementType>
inline constexpr bool operator==(span<ElementType> lhs,
                                 span<OtherElementType> rhs) noexcept {
  return std::equal(lhs.cbegin(), lhs.cend(), rhs.cbegin(), rhs.cend());
}
template <typename ElementType, typename OtherElementType>
inline constexpr bool operator!=(span<ElementType> lhs,
                                 span<OtherElementType> rhs) noexcept {
  return !(lhs == rhs);
}
template <typename ElementType, typename OtherElementType>
inline constexpr bool operator<(span<ElementType> lhs,
                                span<OtherElementType> rhs) noexcept {
  return std::lexicographical_compare(lhs.cbegin(), lhs.cend(), rhs.cbegin(),
                                      rhs.cend());
}
template <typename ElementType, typename OtherElementType>
inline constexpr bool operator<=(span<ElementType> lhs,
                                 span<OtherElementType> rhs) noexcept {
  return !(rhs < lhs);
}
template <typename ElementType, typename OtherElementType>
inline constexpr bool operator>(span<ElementType> lhs,
                                span<OtherElementType> rhs) noexcept {
  return rhs < lhs;
}
template <typename ElementType, typename OtherElementType>
inline constexpr bool operator>=(span<ElementType> lhs,
                                 span<OtherElementType> rhs) noexcept {
  return !(lhs < rhs);
}

template <typename ElementType>
inline constexpr span<ElementType> make_span(ElementType* data,
                                             std::size_t size) noexcept {
  return span<ElementType>(data, size);
}
template <typename ElementType>
inline constexpr span<ElementType> make_span(ElementType* begin,
                                             ElementType* end) noexcept {
  return span<ElementType>(begin, end);
}
template <typename ElementType, std::size_t ArraySize>
constexpr span<ElementType> make_span(ElementType (&array)[ArraySize])
    noexcept {
  return span<ElementType>(array);
}

}  // namespace berrydb

#endif  // BERRYDB_INCLUDE_BERRYDB_SPAN_H_
