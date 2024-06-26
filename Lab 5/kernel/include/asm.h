#ifndef __ASM_H__
#define __ASM_H__


#define asm_read_sysregister(r) ({                  \
    uint64_t __val;                                 \
    asm volatile("mrs %0, " #r : "=r" (__val));     \
    __val;                                          \
})

#define asm_write_sysregister(r, __val) ({          \
    asm volatile("msr " #r ", %0" :: "r" (__val));  \
})



#endif