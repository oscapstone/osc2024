#ifndef TIMER_UTILS_H
#define TIMER_UTILS_H

#include <stdint.h>

#define read_sysreg(sys) ({   \
    uint64_t _val;            \
    asm volatile ("mrs %0, " #sys : "=r" (_val)); \
    _val;                     \
})

#define write_sysreg(sys, _val) ({    \
    asm volatile ("msr " #sys ", %0" :: "r" (_val)); \
})

#define read_gpreg(gp) ({     \
    uint64_t _val;            \
    asm volatile ("mov %0, " #gp :"=r" (_val)); \
    _val;                     \
})

#define write_gpreg(gp, _val) ({     \
    asm volatile ("mov " #gp ", %0" :: "r" (_val)); \
})

#endif