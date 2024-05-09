#include "string.h"

int strcmp(const char *str1, const char *str2)
{
    while (*str1 && *str1 == *str2)
        str1++, str2++;
    return *(unsigned char *)str1 - *(unsigned char *)str2;
}

int memcmp(const void *str1, const void *str2, int n)
{
    const unsigned char *a = str1, *b = str2;
    while (n-- > 0) {
        if (*a != *b)
            return *a - *b;
        a++;
        b++;
    }
    return 0;
}

char *strncpy(char *dest, const char *src, int n)
{
    while (n-- && (*dest++ = *src++))
        ;
    return dest;
}

int strlen(const char *str)
{
    int len = 0;
    while (*str++ != '\0')
        len++;
    return len;
}

char *strcat(char *dest, const char *src)
{
    char *d = dest;

    // Move the pointer to the end of the dest string
    while (*d != '\0')
        d++;
    // Copy the src string to the end of the dest string
    while (*src != '\0')
        *d++ = *src++;
    // Add the null terminator
    *d = '\0';

    return dest;
}

void *memcpy(void *dest, const void *src, int n)
{
    char *d = dest;
    const char *s = src;
    while (n--)
        *d++ = *s++;
    return dest;
}