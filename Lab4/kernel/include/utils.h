#ifndef UTILS_H
#define UTILS_H

#include "bool.h"

extern void put32(unsigned long addr, unsigned int val);
extern unsigned int get32(unsigned long addr);
extern void delay(unsigned long cl);
extern unsigned int get_el(void);

#define BITS_PER_BYTE 8
#define BITS_PER_LONG 64
#define BIT_MASK(nr)  (1UL << ((nr) % BITS_PER_LONG))
#define BIT_WORD(nr)  ((nr) / BITS_PER_LONG)

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

static inline bool test_bit(unsigned int nr, unsigned long* addr)
{
    return 1UL & (addr[BIT_WORD(nr)] >> ((nr) & (BITS_PER_LONG - 1)));
}

#endif /* UTILS_H */
