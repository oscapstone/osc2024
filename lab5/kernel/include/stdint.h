typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef signed short int16_t;
typedef unsigned short uint16_t;
typedef signed int int32_t;
typedef unsigned int uint32_t;
typedef signed long int64_t;
typedef unsigned long uint64_t;
typedef __SIZE_TYPE__ size_t;
typedef uint64_t ssize_t;

/* limits of integral types */

#define        INT8_MIN  (-128)
#define       INT16_MIN  (-32767-1)
#define       INT32_MIN  (-2147483647-1)
#define       INT64_MIN  (-9223372036854775807LL-1)

#define        INT8_MAX  (127)
#define       INT16_MAX  (32767)
#define       INT32_MAX  (2147483647)
#define       INT64_MAX  (9223372036854775807LL)

#define       UINT8_MAX  (255)
#define      UINT16_MAX  (65535)
#define      UINT32_MAX  (4294967295U)
#define      UINT64_MAX  (18446744073709551615ULL)