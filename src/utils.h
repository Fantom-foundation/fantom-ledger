#ifndef FANTOM_LEDGER_UTILS_H
#define FANTOM_LEDGER_UTILS_H

#include <os.h>
#include "assert.h"

// Does not compile if x is pointer of some kind.
// See http://zubplot.blogspot.com/2015/01/gcc-is-wonderful-better-arraysize-macro.html
#define ARRAY_NOT_A_PTR(x) \
    (sizeof(__typeof__(int[1 - 2 * \
        !!__builtin_types_compatible_p(__typeof__(x), \
        __typeof__(&x[0]))])) * 0)


// Safe array length, does not compile if you accidentally supply a pointer.
#define ARRAY_LEN(arr) (sizeof(arr) / sizeof((arr)[0]) + ARRAY_NOT_A_PTR(arr))

// Does not compile if x *might* be a pointer of some kind.
// Might produce false positives on small structs.
// Note: ARRAY_NOT_A_PTR does not compile if arg is a struct so this is a workaround
#define SIZEOF_NOT_A_PTR(var) (sizeof(__typeof(int[0 - (sizeof(var) == sizeof((void *)0))])) * 0)

// Safe version of SIZEOF, does not compile if you accidentally supply a pointer.
#define SIZEOF(var) (sizeof(var) + SIZEOF_NOT_A_PTR(var))

// Given that memset is root of many problems, a bit of paranoia is good.
#define MEMCLEAR(ptr, expected_type) \
    do { \
        ASSERT(sizeof(expected_type) == sizeof(*(ptr))); \
        memset(ptr, 0, sizeof(expected_type)); \
    } while(0)

// Helper macro to check APDU request parameters.
#define VALIDATE(cond, error) \
    do {\
        if (!(cond)) { \
            THROW(error); \
        } \
    } while(0)

// Note: unused removes unused warning but does not warn if you suddenly
// start using such variable. deprecated deals with that.
#define MARK_UNUSED __attribute__ ((unused, deprecated))

// Any buffer claiming to be longer than this is a bug
// We have only 4KB of memory available
#define MAX_BUFFER_SIZE 1024

#endif //FANTOM_LEDGER_UTILS_H
