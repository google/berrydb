// Copyright 2018 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BERRYDB_PLATFORM_COMPILER_H_
#define BERRYDB_PLATFORM_COMPILER_H_

// Embedders who use non-supported compilers may need to replace the macro
// definitions below.
//
// HAS_CPP_ATTRIBUTE is a backport of C++20's __has_cpp_attribute.
//
// MAYBE_UNUSED is a backport of C++17's [[maybe_unused]]. This backport may
// only be used as a prefix of local variables or arguments.
//
// NODISCARD is a backport of C++17's [[nodiscard]].
//
// LIKELY(predicate) is a partial backport of C++20's [[likely]]. This backport
// may only be used as if(LIKELY(predicate)).
//
// UNLIKELY(predicate) is a partial backport of C++20's [[unlikely]]. This
// backport may only be used as if(UNLIKELY(predicate)).
//
// BUILTIN_ASSUME(predicate) asks the optimizer to assume that the predicate is
// true. The predicate may be evaluated, so it must not have side effects.
// Executing of a BUILTIN_ASSUME with a false predicate has undefined behavior.
// This macro is used to implement BERRYDB_ASSUME() variants, which result in
// DCHECKs on debug builds.
//
// BUILTIN_UNREACHABLE() informs the compiler that the current location will
// never be executed. Executing a BUILTIN_UNREACHABLE() has undefined behavior.
// This macro is used to implement BERRYDB_UNREACHABLE(), which results in
// DCHECKs on debug builds.
//
// ALWAYS_INLINE asks the compiler to ignore its heuristics and inline a method.
// This macro must be used instead of the inline keyword, not in addition to it.
//
// NOINLINE asks the compiler to never inline a method. It is useful for error
// handling code.

#if !defined(HAS_CPP_ATTRIBUTE)
#if defined(__has_cpp_attribute)
#define HAS_CPP_ATTRIBUTE(x) __has_cpp_attribute(x)
#else
#define HAS_CPP_ATTRIBUTE(x) 0
#endif  // !defined(__has_cpp_attribute)
#endif  // !defined(HAS_CPP_ATTRIBUTE)

#if !defined(NODISCARD)
#if HAS_CPP_ATTRIBUTE(nodiscard) && __cplusplus >= 201603L
#define NODISCARD [[nodiscard]]
#elif defined(__clang__) || defined(__GNUC__)
#define NODISCARD __attribute__((warn_unused_result))
#elif defined(_MSC_VER)
#define NODISCARD _Check_return_
#else
#define NODISCARD
#endif  // HAS_CPP_ATTRIBUTE(nodiscard)
#endif  // !defined(NODISCARD)

#if !defined(MAYBE_UNUSED)
// GCC does not parse [[maybe_unused]] correctly. Fall back to older attribute.
// See https://gcc.gnu.org/bugzilla/show_bug.cgi?id=81429
#if HAS_CPP_ATTRIBUTE(maybe_unused) && __cplusplus >= 201603L && \
    (!defined(__GNUC__) || defined(__clang__))
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

#if !defined(LIKELY)
#if HAS_CPP_ATTRIBUTE(likely) && __cplusplus >= 201803L
#define LIKELY(x) [[likely]] (x)
#elif defined(__clang__) || defined(__GNUC__)
#define LIKELY(x) __builtin_expect(!!(x), true)
#else
#define LIKELY(x) (x)
#endif  // HAS_CPP_ATTRIBUTE(likely)
#endif  // !defined(BERYYDB_LIKELY)

#if !defined(UNLIKELY)
#if HAS_CPP_ATTRIBUTE(unlikely) && __cplusplus >= 201803L
#define UNLIKELY(x) [[unlikely]] (x)
#elif defined(__clang__) || defined(__GNUC__)
#define UNLIKELY(x) __builtin_expect(!!(x), false)
#else
#define UNLIKELY(x) (x)
#endif  // HAS_CPP_ATTRIBUTE(unlikely)
#endif  // !defined(UNLIKELY)

#if !defined(BUILTIN_ASSUME)
#if defined(__clang__)
#define BUILTIN_ASSUME(x) if(x) {} else __builtin_unreachable()
// Clang also supports the alternative below, which is closer to our intent.
// Sadly, we can't use this alternative because it generates warnings.
// #define BUILTIN_ASSUME(x) __builtin_assume(x)
#elif defined(__GNUC__)
#define BUILTIN_ASSUME(x) if(x) {} else __builtin_unreachable()
#elif defined(_MSC_VER)
// We should be able to use the MSVC __assume statement, as shown below.
// However, the current MSVC version (19.16.27025.1) generates bad code, even
// though the assumptions are all true.
//
// #define BUILTIN_ASSUME(x) __assume(x)
#define BUILTIN_ASSUME(x)
#else
#define BUILTIN_ASSUME(x)
#endif  // defined(__clang__)
#endif  // !defined(BUILTIN_ASSUME)

#if !defined(BUILTIN_UNREACHABLE)
#if defined(__clang__) || defined(__GNUC__)
#define BUILTIN_UNREACHABLE() __builtin_unreachable()
#elif defined(_MSC_VER)
#define BUILTIN_UNREACHABLE() __assume(0)
#else
#define BUILTIN_UNREACHABLE()
#endif  // defined(__clang__) || defined(__GNUC__)
#endif  // !defined(BUILTIN_UNREACHABLE)

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

