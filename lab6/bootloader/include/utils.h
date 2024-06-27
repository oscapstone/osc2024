#ifndef	_UTILS_H
#define	_UTILS_H

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

extern void delay ( unsigned long);
extern void put32 ( unsigned long, unsigned int );
extern unsigned int get32 ( unsigned long );

#endif  /*_UTILS_H */
