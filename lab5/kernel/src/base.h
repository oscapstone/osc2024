#ifndef BASE_H
#define BASE_H

#include <stddef.h>

// static in C mean only for specific c file only can not access from other file using header (expect function pointer)

typedef unsigned char               U8;
typedef unsigned short              U16;
typedef unsigned int                U32;
typedef unsigned long long          U64;

typedef unsigned long long          pd_t;


typedef volatile U32                REG32;

typedef unsigned long long          UPTR;


typedef char                        BOOL;

#define TRUE    1
#define FALSE   0


// because the c in aling in 32 bit, if you don't want your struct having padding
// define this at the end of your struct
// struct TestStruct { [data] } PACKED;
// but you need to enable the mmu to use this techique
#define PACKED __attribute((__packed__))

#ifdef NS_DEBUG
#define NS_DPRINT(...) printf(__VA_ARGS__)
#else
#define NS_DPRINT(...)
#endif

#endif