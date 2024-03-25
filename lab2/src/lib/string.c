#include "uart.h"

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
