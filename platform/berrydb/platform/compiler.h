// Copyright 2018 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_PLATFORM_COMPILER_H_
#define BERRYDB_PLATFORM_COMPILER_H_

// Embedders who use non-supported compilers must replace the macro definitions
// below.

// HAS_CPP_ATTRIBUTE is a backport of C++20's __has_cpp_attribute.
#if !defined(HAS_CPP_ATTRIBUTE)
#if defined(__has_cpp_attribute)
#define HAS_CPP_ATTRIBUTE(x) __has_cpp_attribute(x)
#else
#define HAS_CPP_ATTRIBUTE(x) 0
#endif  // !defined(__has_cpp_attribute)
#endif  // !defined(HAS_CPP_ATTRIBUTE)

// NODISCARD is a backport of C++17's [[nodiscard]].
#if !defined(NODISCARD)
#if HAS_CPP_ATTRIBUTE(nodiscard)
#define NODISCARD [[nodiscard]]
#elif defined(__clang__) || defined(__GNUC__)
#define NODISCARD __attribute__((warn_unused_result))
#elif defined(_MSC_VER)
#define NODISCARD _Check_return_
#else
#define NODISCARD
#endif  // HAS_CPP_ATTRIBUTE(nodiscard)
#endif  // !defined(NODISCARD)

// MAYBE_UNUSED is a backport of C++17's [[maybe_unused]]. This backport may
// only be used as a prefix of local variables or arguments.
#if !defined(MAYBE_UNUSED)
#if HAS_CPP_ATTRIBUTE(maybe_unused)
#define MAYBE_UNUSED [[maybe_unused]]
#elif defined(__clang__) || defined(__GNUC__)
#define MAYBE_UNUSED __attribute__((__unused__))
#elif defined(_MSC_VER)
// Visual Studio's C++ compiler doesn't appear to have a pre-[[maybe_unused]]
// attribute. A heavy-handed workaround is to disable the relevant warnings.
#pragma warning(disable: 4100)  // unused argument
#pragma warning(disable: 4189)  // unused local variable
#define MAYBE_UNUSED
#else
// Users of other compilers will have to disable the warnings.
#define MAYBE_UNUSED
#endif  // HAS_CPP_ATTRIBUTE(maybe_unused)
#endif  // !defined(MAYBE_UNUSED)

// LIKELY(predicate) is a partial backport of C++20's [[likely]]. This backport
// may only be used as if(LIKELY(predicate)).
#if !defined(LIKELY)
#if HAS_CPP_ATTRIBUTE(likely)
#define LIKELY(x) [[likely]] (x)
#elif defined(__clang__) || defined(__GNUC__)
#define LIKELY(x) __builtin_expect(!!(x), true)
#else
#define LIKELY(x) (x)
#endif  // HAS_CPP_ATTRIBUTE(likely)
#endif  // !defined(BERYYDB_LIKELY)

// UNLIKELY(predicate) is a partial backport of C++20's [[unlikely]]. This
// backport may only be used as if(UNLIKELY(predicate)).
#if !defined(UNLIKELY)
#if HAS_CPP_ATTRIBUTE(unlikely)
#define UNLIKELY(x) [[unlikely]] (x)
#elif defined(__clang__) || defined(__GNUC__)
#define UNLIKELY(x) __builtin_expect(!!(x), false)
#else
#define UNLIKELY(x) (x)
#endif  // HAS_CPP_ATTRIBUTE(unlikely)
#endif  // !defined(UNLIKELY)

// ASSUME(predicate) asks the optimizer to assume that the predicate is true.
// The predicate may be evaluated, so it must not have side effects. Passing in
// a false predicate results in undefined behavior.
#if !defined(ASSUME)
#if defined(__clang__)
#define ASSUME(x) if(x) {} else __builtin_unreachable()
// Clang also supports the alternative below, which is closer to our intent.
// Sadly, we can't use this alternative because it generates warnings.
// #define ASSUME(x) __builtin_assume(x)
#elif defined(__GNUC__)
#define ASSUME(x) if(x) {} else __builtin_unreachable()
#elif defined(_MSC_VER)
#define ASSUME(x) __assume(x)
#else
#define ASSUME(x) assert(x)
#endif  // defined(__clang__)
#endif  // !defined(ASSUME)

// UNREACHABLE() informs the compiler that the current location will never be
// executed. Executing an UNREACHABLE() results in undefined behavior.
#if !defined(UNREACHABLE)
#if defined(__clang__) || defined(__GNUC__)
#define UNREACHABLE() __builtin_unreachable()
#elif defined(_MSC_VER)
#define UNREACHABLE() __assume(0)
#else
#define UNREACHABLE() assert(false)
#endif  // defined(__clang__) || defined(__GNUC__)
#endif  // !defined(UNREACHABLE)

// ALWAYS_INLINE asks the compiler to ignore its heuristics and inline a method.
// This macro must be used instead of the inline keyword, not in addition to it.
#if !defined(ALWAYS_INLINE)
#if defined(__clang__) || defined(__GNUC__)
// GCC warns if the attribute is used without the "inline" keyword.
#define ALWAYS_INLINE __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
// MSVC warns if "inline" is used together with __forceinline.
#define ALWAYS_INLINE __forceinline
#else
#define ALWAYS_INLINE inline
#endif  // defined(__clang__) || defined(__GNUC__)
#endif  // !defined(ALWAYS_INLINE)

// NOINLINE asks the compiler to never inline a method. It is useful for error
// handling code.
#if !defined(NOINLINE)
#if defined(__clang__) || defined(__GNUC__)
#define NOINLINE __attribute__((noinline))
#elif defined(_MSC_VER)
#define NOINLINE __declspec(noinline)
#else
#define NOINLINE
#endif  // defined(__clang__) || defined(__GNUC__)
#endif  // !defined(NOINLINE)

#endif  // BERRYDB_PLATFORM_COMPILER_H_

