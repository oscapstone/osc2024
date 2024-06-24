#include "string.h"
#include <stddef.h>

unsigned int vsprintf(char *dst, char *fmt, __builtin_va_list args)
{
    long int arg;
    int len, sign, i;
    char *p, *orig = dst, tmpstr[19];

    // failsafes
    if (dst == (void *)0 || fmt == (void *)0) {
        return 0;
    }

    // main loop
    arg = 0;
    while (*fmt) {
        if (dst - orig > VSPRINT_MAX_BUF_SIZE - 0x10) {
            return -1;
        }
        // argument access
        if (*fmt == '%') {
            fmt++;
            // literal %
            if (*fmt == '%') {
                goto put;
            }
            len = 0;
            // size modifier
            while (*fmt >= '0' && *fmt <= '9') {
                len *= 10;
                len += *fmt - '0';
                fmt++;
            }
            // skip long modifier
            if (*fmt == 'l') {
                fmt++;
            }
            // character
            if (*fmt == 'c') {
                arg = __builtin_va_arg(args, int);
                *dst++ = (char)arg;
                fmt++;
                continue;
            }
            else
                // decimal number
                if (*fmt == 'd') {
                    arg = __builtin_va_arg(args, int);
                    // check input
                    sign = 0;
                    if ((int)arg < 0) {
                        arg *= -1;
                        sign++;
                    }
                    if (arg > 99999999999999999L) {
                        arg = 99999999999999999L;
                    }
                    // convert to string
                    i = 18;
                    tmpstr[i] = 0;
                    do {
                        tmpstr[--i] = '0' + (arg % 10);
                        arg /= 10;
                    } while (arg != 0 && i > 0);
                    if (sign) {
                        tmpstr[--i] = '-';
                    }
                    if (len > 0 && len < 18) {
                        while (i > 18 - len) {
                            tmpstr[--i] = ' ';
                        }
                    }
                    p = &tmpstr[i];
                    goto copystring;
                }
                else if (*fmt == 'x') {
                    arg = __builtin_va_arg(args, long int);
                    i = 16;
                    tmpstr[i] = 0;
                    do {
                        char n = arg & 0xf;
                        tmpstr[--i] = n + (n > 9 ? 0x37 : 0x30);
                        arg >>= 4;
                    } while (arg != 0 && i > 0);
                    if (len > 0 && len <= 16) {
                        while (i > 16 - len) {
                            tmpstr[--i] = '0';
                        }
                    }
                    p = &tmpstr[i];
                    goto copystring;
                }
                else if (*fmt == 's') {
                    p = __builtin_va_arg(args, char *);
                copystring:
                    if (p == (void *)0) {
                        p = "(null)";
                    }
                    while (*p) {
                        *dst++ = *p++;
                    }
                }
        }
        else {
        put:
            *dst++ = *fmt;
        }
        fmt++;
    }
    *dst = 0;
    return dst - orig;
}

unsigned int sprintf(char *dst, char *fmt, ...)
{
    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    unsigned int r = vsprintf(dst, fmt, args);
    __builtin_va_end(args);
    return r;
}

unsigned long long strlen(const char *str)
{
    size_t count = 0;
    while ((unsigned char)*str++)
        count++;
    return count;
}

int strcmp(const char *p1, const char *p2)
{
    const unsigned char *s1 = (const unsigned char *)p1;
    const unsigned char *s2 = (const unsigned char *)p2;
    unsigned char c1, c2;

    do {
        c1 = (unsigned char)*s1++;
        c2 = (unsigned char)*s2++;
        if (c1 == '\0')
            return c1 - c2;
    } while (c1 == c2);
    return c1 - c2; // return 0 if equal
}

int strncmp(const char *s1, const char *s2, unsigned long long n)
{
    unsigned char c1 = '\0';
    unsigned char c2 = '\0';
    if (n >= 4) {
        size_t n4 = n >> 2;
        do {
            c1 = (unsigned char)*s1++;
            c2 = (unsigned char)*s2++;
            if (c1 == '\0' || c1 != c2)
                return c1 - c2;
            c1 = (unsigned char)*s1++;
            c2 = (unsigned char)*s2++;
            if (c1 == '\0' || c1 != c2)
                return c1 - c2;
            c1 = (unsigned char)*s1++;
            c2 = (unsigned char)*s2++;
            if (c1 == '\0' || c1 != c2)
                return c1 - c2;
            c1 = (unsigned char)*s1++;
            c2 = (unsigned char)*s2++;
            if (c1 == '\0' || c1 != c2)
                return c1 - c2;
        } while (--n4 > 0);
        n &= 3;
    }
    while (n > 0) {
        c1 = (unsigned char)*s1++;
        c2 = (unsigned char)*s2++;
        if (c1 == '\0' || c1 != c2)
            return c1 - c2;
        n--;
    }
    return c1 - c2;
}

char *memcpy(void *dest, const void *src, unsigned long long len)
{
    char *d = dest;
    const char *s = src;
    while (len--)
        *d++ = *s++;
    return dest;
}

char *strcpy(char *dest, const char *src) { return memcpy(dest, src, strlen(src) + 1); }

void *memset(void *s, int c, size_t n)
{
    char *start = s;
    for (size_t i = 0; i < n; i++) {
        start[i] = c;
    }

    return s;
}

char *strchr(register const char *s, int c)
{
    do {
        if (*s == c) {
            return (char *)s;
        }
    } while (*s++);
    return (0);
}

char *str_SepbySpace(char *head)
{
    char *end;
    while (1) {
        if (*head == '\0') {
            end = head;
            break;
        }
        if (*head == ' ') {
            *head = '\0';
            end = head + 1;
            break;
        }
        head++;
    }
    return end;
}

// A simple atoi() function
int atoi(char *str)
{
    // Initialize result
    int res = 0;

    // Iterate through all characters
    // of input string and update result
    // take ASCII character of corresponding digit and
    // subtract the code from '0' to get numerical
    // value and multiply res by 10 to shuffle
    // digits left to update running total
    for (int i = 0; str[i] != '\0'; ++i) {
        if (str[i] > '9' || str[i] < '0')
            return res;
        res = res * 10 + str[i] - '0';
    }

    // return result.
    return res;
}

char *strcat(char *dest, const char *src)
{
    strcpy(dest + strlen(dest), src);
    return dest;
}