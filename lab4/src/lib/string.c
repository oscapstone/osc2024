#include "string.h"
#include "uart.h"
#include <stddef.h>

#define VSPRINTF_BUF_SIZE 256

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
        if (dst - orig > VSPRINTF_BUF_SIZE - 0x10) {
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

int strlen(const char *s)
{
    const char *sc;
    for (sc = s; *sc != '\0'; ++sc)
        ;
    return sc - s;
}

int strcmp(const char *cs, const char *ct)
{
    unsigned char c1, c2;
    while (1) {
        c1 = *cs++;
        c2 = *ct++;
        if (c1 != c2)
            return c1 < c2 ? -1 : 1;
        if (!c1)
            break;
    }
    return 0;
}

int strncmp(const char *cs, const char *ct, int count)
{
    while (count-- && *cs && (*cs == *ct)) {
        cs++;
        ct++;
    }
    if (count == (int)-1)
        return 0;

    return *(const unsigned char *)cs - *(const unsigned char *)ct;
}

char *strcpy(char *dest, const char *src)
{
    char *tmp = dest;
    while ((*dest++ = *src++) != '\0')
        ;
    return tmp;
}

char *strtok(char *str, const char *delimiters)
{
    static char *buffer = 0;
    if (str != 0)
        buffer = str;
    if (buffer == 0)
        return 0;

    char *start = buffer;
    while (*buffer != '\0') {
        const char *delim = delimiters;
        while (*delim != '\0') {
            if (*buffer == *delim) {
                *buffer = '\0';
                buffer++;
                if (start != buffer)
                    return start;
                else {
                    start++;
                    break;
                }
            }
            delim++;
        }
        if (*delim == '\0')
            buffer++;
    }
    if (start == buffer)
        return 0;
    else
        return start;
}

unsigned int parse_hex_str(char *s, unsigned int max_len)
{
    unsigned int r = 0;
    for (unsigned int i = 0; i < max_len; i++) {
        r *= 16;
        if ('0' <= s[i] && s[i] <= '9')
            r += s[i] - '0';
        else if ('a' <= s[i] && s[i] <= 'f')
            r += s[i] - 'a' + 10;
        else if ('A' <= s[i] && s[i] <= 'F')
            r += s[i] - 'A' + 10;
        else
            break;
    }
    return r;
}

long long atoi(const char *s)
{
    long long r = 0;
    while (*s) {
        r = r * 10 + *s - '0';
        s++;
    }
    return r;
}