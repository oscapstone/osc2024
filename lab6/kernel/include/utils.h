#ifndef	_BOOT_H
#define	_BOOT_H

extern void put32 ( unsigned long, unsigned int );
extern unsigned int get32 ( unsigned long );

#define write_sysreg(r, __val) ({                \
    asm volatile("msr " #r ", %0" ::"r"(__val)); \
})

#define read_sysreg(r) ({                       \
    unsigned long __val;                        \
    asm volatile("mrs %0, " #r : "=r" (__val)); \
    __val;                                      \
})

#endif  /*_BOOT_H */
