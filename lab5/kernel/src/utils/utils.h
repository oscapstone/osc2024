#ifndef UTILS_H
#define UTILS_H

#include "base.h"


unsigned long utils_atoi(const char *s, int char_size);
/**
 * transfer decial number string to unsigned int
*/
U64 utils_atou_dec(const char *s, int char_size);
void utils_align(void *size, unsigned int s);
int utils_strncmp(const char* str1, const char* str2, unsigned int len);
U32 utils_transferEndian(U32 value);
U32 utils_align_up(U32 size, int alignment);
U64 utils_strlen(const char *s);
U64 utils_highestOneBit(U64 number);
void utils_char_fill(char* dst, char* content, U64 size);

// in assembly file utilsASM.S
void utils_delay(U64 cycle);
U32 utils_get_el();

#define utils_read_sysreg(r) ({        \
    unsigned long __val;         \
    asm volatile("mrs %0, " #r   \
                 : "=r"(__val)); \
    __val;                       \
})

#define utils_write_sysreg(r, __val) ({                  \
	asm volatile("msr " #r ", %0" :: "r" (__val)); \
})

#endif