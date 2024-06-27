#ifndef	_UTILS_H
#define	_UTILS_H


#include <stdint.h>

#define PA2VA(x) (((uint64_t)(x)) | 0xffff000000000000)
#define VA2PA(x) (((uint64_t)(x)) & 0x0000ffffffffffff)
#define ALIGN(num, base) ((num + base - 1) & ~(base - 1))


// Reference from https://elixir.bootlin.com/linux/latest/source/tools/lib/perf/mmap.c#L299
#define read_sysreg(r) ({                       \
    uint64_t __val;                               \
    asm volatile("mrs %0, " #r : "=r" (__val)); \
    __val;                                      \
})

// Reference from https://elixir.bootlin.com/linux/latest/source/arch/arm64/include/asm/sysreg.h#L1281
#define write_sysreg(r, v) do {    \
    uint64_t __val = (uint64_t)(v);          \
    asm volatile("msr " #r ", %x0" \
             : : "rZ" (__val));    \
} while (0)

#define set_page_table(thread) do {               \
    asm volatile(                               \
        "mov x9, %0\n"                          \
        "and x9, x9, #0x0000ffffffffffff\n"     \
        "dsb ish\n"                             \
        "msr ttbr0_el1, x9\n"                   \
        "tlbi vmalle1is\n"                      \
        "dsb ish\n"                             \
        "isb\n"                                 \
        :: "r" (thread->page_table)               \
    );                                          \
} while (0)

extern void put32 ( unsigned long, unsigned int );
extern unsigned int get32 ( unsigned long );

#endif  /*_UTILS_H */
