#include "utils.h"
#include <stddef.h>

unsigned long long strlen(const char *str)
{
    size_t count = 0;
    while((unsigned char)*str++)count++;
    return count;
}

int strcmp(const char* p1, const char* p2)
{
    const unsigned char *s1 = (const unsigned char*) p1;
    const unsigned char *s2 = (const unsigned char*) p2;
    unsigned char c1, c2;

    do {
        c1 = (unsigned char) *s1++;
        c2 = (unsigned char) *s2++;
        if ( c1 == '\0' ) return c1 - c2;
    } while ( c1 == c2 );
    return c1 - c2;
}

// 比較兩個字串中的前 n 個字符是否相等
int strncmp (const char *s1, const char *s2, unsigned long long n)
{
    unsigned char c1 = '\0';
    unsigned char c2 = '\0';
    if (n >= 4)
    {
        size_t n4 = n >> 2;
        do
        {
            c1 = (unsigned char) *s1++;
            c2 = (unsigned char) *s2++;
            if (c1 == '\0' || c1 != c2)
                return c1 - c2;
            c1 = (unsigned char) *s1++;
            c2 = (unsigned char) *s2++;
            if (c1 == '\0' || c1 != c2)
                return c1 - c2;
            c1 = (unsigned char) *s1++;
            c2 = (unsigned char) *s2++;
            if (c1 == '\0' || c1 != c2)
                return c1 - c2;
            c1 = (unsigned char) *s1++;
            c2 = (unsigned char) *s2++;
            if (c1 == '\0' || c1 != c2)
                return c1 - c2;
        } while (--n4 > 0);
        n &= 3;
    }
    while (n > 0)
    {
        c1 = (unsigned char) *s1++;
        c2 = (unsigned char) *s2++;
        if (c1 == '\0' || c1 != c2)
            return c1 - c2;
        n--;
    }
    return c1 - c2;
}

char* memcpy(void *dest, const void *src, unsigned long long len)
{
    char *d = dest;
    const char *s = src;
    while (len--)
        *d++ = *s++;
    return dest;
}
