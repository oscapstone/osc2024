
#include "utils/utils.h"

void utils_delay(unsigned long cycle) {
    register unsigned long r = cycle;
    
    while (r--) {
        asm volatile("nop");
    }
    return;
}

unsigned long utils_atoi(const char *s, int char_size) {
    unsigned long num = 0;
    for (int i = 0; i < char_size; i++) {
        num = num * 16;
        if (*s >= '0' && *s <= '9') {
            num += (*s - '0');
        } else if (*s >= 'A' && *s <= 'F') {
            num += (*s - 'A' + 10);
        } else if (*s >= 'a' && *s <= 'f') {
            num += (*s - 'a' + 10);
        }
        s++;
    }
    return num;
}

void utils_align(void *size, unsigned int s) {
	unsigned long* x = (unsigned long*) size;
	unsigned long mask = s-1;
	*x = ((*x) + mask) & (~mask);
}

int utils_strncmp(const char* s1, const char* s2, unsigned int n) {
	if (n == 0)
		return (0);
	do {
		if (*s1 != *s2++)
			return (*(unsigned char *)s1 - *(unsigned char *)--s2);
		if (*s1++ == 0)
			break;
	} while (--n != 0);
	return (0);
}

U32 utils_transferEndian(U32 value) {
    const U8 *bytes = (const U8 *) &value;
	U32 ret = (U32)bytes[0] << 24 | (U32)bytes[1] << 16 | (U32)bytes[2] << 8 | (U32)bytes[3];
	return ret;
}

U32 utils_align_up(U32 size, int alignment) {
  return (size + alignment - 1) & ~(alignment-1);
}

U64 utils_strlen(const char *s) {
    U64 i = 0;
	while (s[i]) i++;
	return i+1;
}
