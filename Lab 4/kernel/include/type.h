#ifndef __TYPE_H__
#define __TYPE_H__

#include <stdint.h>

#define nullptr 0l

typedef void*       voidptr_t;
typedef void*       void_ptr;
typedef char*       byteptr_t;
typedef char*       byte_ptr;
typedef char*       char_ptr;
typedef char        byte_t;
typedef uint8_t*    uint8ptr_t;
typedef uint8_t*    uint8_ptr;
typedef uint32_t*   uint32ptr_t;
typedef uint32_t*   uint32_ptr;
typedef uint64_t*   uint64ptr_t;
typedef uint64_t*   uint64_ptr;

#define INT32_LEFTMOST_ORDER(x)     ({31 - __builtin_clz(x);})
#define INT32_LEFTMOST(x)           1 << INT32_LEFTMOST_ORDER(x)
#define INT32_RIGHTMOST_ORDER(x)    ({__builtin_ctz(x);})
#define INT32_RIGHTMOST(x)          1 << INT32_RIGHTMOST_ORDER(x)

#define UINT32_ALIGN(v, o)          ({(v) & ~((1 << o) - 1);})
#define UINT32_PADDING(v, o)        ({(v + ((1 << o) - 1)) & ~((1 << o) - 1);})


#define max(a, b) (a > b ? a : b)
#define min(a, b) (a < b ? a : b)

#endif