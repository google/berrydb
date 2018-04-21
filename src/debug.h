
#ifndef BERRYDB_DEBUG_H_
#define BERRYDB_DEBUG_H_

#if defined(_MSC_VER)

#define UNUSED 
#define NO_RETURN
#define NO_INLINE

#elif defined(__clang__) || defined(__GNUC__)

#define UNUSED __attribute__((unused))
#define NO_RETURN __attribute__((noreturn))
#define NO_INLINE __attribute__((noinline))

#endif  // _MSC_VER

#endif  // BERRYDB_DEBUG_H_