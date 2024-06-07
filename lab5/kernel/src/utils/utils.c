
#include "utils/utils.h"
#include "utils/printf.h"

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

U64 utils_atou_dec(const char *s, int char_size) {
    U64 val = 0;
    for (int i = 0; i < char_size; i++) {
        val = val * 10;
        if (*s >= '0' && *s <= '9') {
            val += (*s - '0');
        }
        s++;
    }
    return val;
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

U64 utils_highestOneBit(U64 number) {
    int position = 0;
    
    // If n is 0, return 0
    if (number == 0)
        return 0;
    
    // Right-shift n until it becomes 1
    while (number != 1) {
        number >>= 1; // Right shift by 1 bit
        position++; // Increment position
    }
    
    // Position of highest 1 bit
    return position + 1;
}

void utils_char_fill(char* dst, char* content, U64 size) {

    for (U64 i = 0; i < size; i++) {
        dst[i] = content[i];
    }
}

U32 utils_str2uint_dec(const char *str)
{
    U32 value = 0;

    while (*str >= '0' && *str <= '9')
    {
        value = (value * 10) + (*str - '0');
        ++str;
    }
    return value;
}

U32 utils_read_unaligned_u32(const void* address) {
    const U8* byte_ptr = (const U8*)address;
    U32 value = 0;

    value |= (U32)byte_ptr[0];
    value |= (U32)byte_ptr[1] << 8;
    value |= (U32)byte_ptr[2] << 16;
    value |= (U32)byte_ptr[3] << 24;

    return value;
}

U16 utils_read_unaligned_u16(const void* addr) {
    const U8* byte_ptr = (const U8*)addr;
    U16 value = 0;

    value |= (U16)byte_ptr[0];
    value |= (U16)byte_ptr[1] << 8;

    return value;
}