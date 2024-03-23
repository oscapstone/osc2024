#include "string_utils.h"

int my_strcmp(char *s1, char *s2)
{
    while (*s1 == *s2) {
        s1++;
        s2++;
        if (*s1 == '\0')
            break;
    }
    return *s1 - *s2;
}

unsigned char hexchar2val(char ch)
{
    const char letter = ch & 0x40;
    const char shift = (letter >> 3) | (letter >> 6);
    return (ch + shift) & 0xf;
}

unsigned int hexstr2val(char *str, unsigned int size)
{
    unsigned int result = 0;
    for (unsigned int i = 0; i < size; i++) {
        result = (result << 4) | (hexchar2val(str[i]));
    }
    return result;
}

void mem_align(void *addr, unsigned int number)
{
    unsigned long *x = (unsigned long *)addr;
    unsigned long mask = number - 1;
    *x = (*(x) + mask) & (~mask);
}