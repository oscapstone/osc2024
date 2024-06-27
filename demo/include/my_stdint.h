#ifndef MY_STDINT_H
#define MY_STDINT_H

#define __LP64__ 1 // Define __LP64__ macro to indicate a 64-bit system

typedef signed char         int8_t;
typedef short               int16_t;
typedef int                 int32_t;
typedef long long           int64_t;

typedef unsigned char       uint8_t;
typedef unsigned short      uint16_t;
typedef unsigned int        uint32_t;
typedef unsigned long long  uint64_t;

// Define uintptr_t based on the size of a pointer
#if defined(__LP64__) || defined(_LP64)
typedef unsigned long int uintptr_t;
#else
typedef unsigned int uintptr_t;
#endif

#endif /* _STDINT_H */