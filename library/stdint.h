#ifndef STDINT_H
#define STDINT_H

#ifndef __bool_defined
typedef int                          bool;
#define __bool_defined
#endif

#define true 1
#define false 0

#ifndef __uint8_t_defined
typedef unsigned char                uint8_t;
#define __uint8_t_defined
#endif

#ifndef __uint16_t_defined
typedef unsigned short int           uint16_t;
#define __uint16_t_defined
#endif

#ifndef __uint32_t_defined
typedef unsigned int                 uint32_t;
#define __uint32_t_defined
#endif

#ifndef __uint64_t_defined
typedef unsigned long long int       uint64_t;
#define __uint64_t_defined
#endif

#ifndef __u8_defined
typedef unsigned char                u8;
#define __u8_defined
#endif

#ifndef __u16_defined
typedef unsigned short int           u16;
#define __u16_defined
#endif

#ifndef __u32_defined
typedef unsigned int                 u32;
#define __u32_defined
#endif

#ifndef __u64_defined
typedef unsigned long long int       u64;
#define __u64_defined
#endif

#define ULLONG_MAX	                 (~0ULL)

#endif