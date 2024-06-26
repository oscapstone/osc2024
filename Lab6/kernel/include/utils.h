#ifndef UTILS_H
#define UTILS_H

#include "bool.h"
#include "def.h"


#ifndef __ASSEMBLER__

extern void put32(unsigned long addr, unsigned int val);
extern unsigned int get32(unsigned long addr);
extern void delay(unsigned long cl);
extern unsigned int get_el(void);
extern void set_pgd(unsigned long pgd_addr);

#endif

#define BITS_PER_BYTE 8
#define BITS_PER_LONG 64
#define BIT_MASK(nr)  (1UL << ((nr) % BITS_PER_LONG))
#define BIT_WORD(nr)  ((nr) / BITS_PER_LONG)

#define min(a, b)                      \
    ({                                 \
        __typeof__(a) __a__ = (a);     \
        __typeof__(b) __b__ = (b);     \
        __a__ < __b__ ? __a__ : __b__; \
    })

#define max(a, b)                      \
    ({                                 \
        __typeof__(a) __a__ = (a);     \
        __typeof__(b) __b__ = (b);     \
        __a__ > __b__ ? __a__ : __b__; \
    })

#define get_lowest_set_bit(x) ((x) & -(x))

#define align_down_pow2(x) get_highest_set_bit((x))

static inline void set_bit(unsigned int nr, unsigned long* addr)
{
    unsigned long mask = BIT_MASK(nr);
    unsigned long* p = ((unsigned long*)addr) + BIT_WORD(nr);
    *p |= mask;
}

static inline void clear_bit(unsigned int nr, unsigned long* addr)
{
    unsigned long mask = BIT_MASK(nr);
    unsigned long* p = ((unsigned long*)addr) + BIT_WORD(nr);
    *p &= ~mask;
}


static inline void flip_bit(unsigned int nr, unsigned long* addr)
{
    unsigned long mask = BIT_MASK(nr);
    unsigned long* p = ((unsigned long*)addr) + BIT_WORD(nr);
    *p ^= ~mask;
}

static inline bool test_bit(unsigned int nr, unsigned long* addr)
{
    return 1UL & (addr[BIT_WORD(nr)] >> ((nr) & (BITS_PER_LONG - 1)));
}

static inline size_t get_highest_set_bit(size_t x)
{
    x |= x >> 32;
    x |= x >> 16;
    x |= x >> 8;
    x |= x >> 4;
    x |= x >> 2;
    x |= x >> 1;
    return x ^ (x >> 1);
}

static inline size_t align_up_pow2(size_t x)
{
    x--;
    x |= x >> 32;
    x |= x >> 16;
    x |= x >> 8;
    x |= x >> 4;
    x |= x >> 2;
    x |= x >> 1;
    x++;
    return x;
}


#endif /* UTILS_H */
