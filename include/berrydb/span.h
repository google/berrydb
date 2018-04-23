// Copyright 2018 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This is a subset of std::span from C++ 2a. When compiler support becomes
// sufficiently advanced, this file will be reduced to a "using std::span".

#include <algorithm>
#include <cassert>
#include <iterator>
#include <type_traits>

namespace berrydb {

template <typename ElementType>
class span {
 public:
  using element_type = ElementType;
  using value_type = std::remove_cv_t<ElementType>;
  using pointer = ElementType*;
  using reference = ElementType&;
  using iterator = ElementType*;
  using const_iterator = const ElementType*;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  inline constexpr span() noexcept : data_(nullptr), size_(0) {}
  inline constexpr span(ElementType* data, size_t size) noexcept
      : data_(data), size_(size) {}

  // The template is a workaround for disqualifying this constructor in the
  // lookup for span(pointer, 0).
  template <typename _ = void>
  inline constexpr span(ElementType* begin, ElementType* end)
      : data_(begin), size_(end - begin) {
    assert(begin <= end);
  }

  // TODO(pwnall): Add container and array constructors, if/when necessary.

  inline constexpr span(const span& other) noexcept = default;
  inline constexpr span& operator=(const span& other) noexcept = default;

  // TODO(pwnall): Switch to std::is_convertible_v when migrating to C++17.
  template <
      typename OtherElementType,
      typename _ = std::enable_if_t<std::is_convertible<
          OtherElementType (*)[], ElementType (*)[]>::value>>
  inline constexpr span(const span<OtherElementType>& other)
      : data_(other.data()), size_(other.size()) {}

  template <
      size_t ArraySize,
      typename _ = std::enable_if_t<true>>
  inline constexpr span(ElementType (&array)[ArraySize]) noexcept :
      data_(array), size_(ArraySize) {}

  inline ~span() noexcept = default;

  inline constexpr span first(size_t count) const noexcept {
    assert(count <= size_);
    return span(data_, count);
  }
  inline constexpr span last(size_t count) const noexcept {
    assert(count <= size_);
    return span(data_ + (size_ - count), count);
  }
  inline constexpr span subspan(size_t pos, size_t count = -1) const noexcept {
    // Value of std::dynamic_extent.
    size_t dynamic_extent = static_cast<size_t>(-1);
    assert(pos <= size_);
    assert(count == dynamic_extent || count <= size_ - pos);
    return span(data_ + pos, (count == dynamic_extent) ? size_ - pos : count);
  }

  inline constexpr size_t size() const noexcept { return size_; }
  inline constexpr size_t size_bytes() const noexcept {
    return size_ * sizeof(ElementType);
  }
  inline constexpr bool empty() const noexcept { return !size_; }

  inline constexpr ElementType& operator[](size_t index) const noexcept {
    assert(index < size_);
    return data_[index];
  }
  inline constexpr ElementType& operator()(size_t index) const noexcept {
    assert(index < size_);
    return data_[index];
  }
  inline constexpr ElementType* data() const noexcept { return data_; }

  inline constexpr iterator begin() const noexcept { return data_; }
  inline constexpr iterator end() const noexcept { return data_ + size_; }
  inline constexpr const_iterator cbegin() const noexcept { return begin(); }
  inline constexpr const_iterator cend() const noexcept { return end(); }

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
  size_t size_;
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
inline constexpr span<ElementType> make_span(ElementType* data, size_t size)
    noexcept {
  return span<ElementType>(data, size);
}
template <typename ElementType>
inline constexpr span<ElementType> make_span(ElementType* begin,
                                             ElementType* end) noexcept {
  return span<ElementType>(begin, end);
}
template <typename ElementType, size_t ArraySize>
constexpr span<ElementType> make_span(ElementType (&array)[ArraySize])
    noexcept {
  return span<ElementType>(array);
}

}  // namespace berrydb
