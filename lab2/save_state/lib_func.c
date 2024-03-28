#include "lib_func.h"
#include "definitions.h"

int strcmp(char *s1, char *s2)
{
    while (*s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

int strncmp(const char *s1, const char *s2, size_t n)
{
    while (n-- > 0 && *s1 && *s1 == *s2)
    {
        s1++;
        s2++;
    }
    return n == (size_t)-1 ? 0 : *(unsigned char *)s1 - *(unsigned char *)s2;
}

void memset(void *ptr, int value, size_t num)
{
    unsigned char *p = ptr;
    while (num--)
    {
        *p++ = (unsigned char)value;
    }
}

int memcmp(void *s1, void *s2, int n)
{
    unsigned char *a = s1, *b = s2;
    while (n-- > 0)
    {
        if (*a != *b)
        {
            return *a - *b;
        }
        a++;
        b++;
    }
    return 0;
}

int hex2bin(char *s, int n)
{
    int r = 0;
    while (n-- > 0)
    {
        r <<= 4;
        if (*s >= '0' && *s <= '9')
            r += *s - '0';
        else if (*s >= 'a' && *s <= 'f')
            r += *s - 'a' + 10;
        else if (*s >= 'A' && *s <= 'F')
            r += *s - 'A' + 10;
        s++;
    }
    return r;
}

size_t strlen(const char *s)
{
    const char *p = s;
    while (*p)
    {
        ++p;
    }
    return p - s;
}

char *strchr(const char *s, int c)
{
    while (*s != (char)c)
    {
        if (!*s++)
        {
            return NULL;
        }
    }
    return (char *)s;
}

char *strrchr(const char *s, int c)
{
    const char *last = NULL;
    do
    {
        if (*s == (char)c)
        {
            last = s;
        }
    } while (*s++);
    return (char *)last;
}

size_t strspn(const char *str1, const char *str2)
{
    const char *p = str1;
    const char *spanp;
    char c, sc;

    // Count characters in str1 that are in str2
    while ((c = *p++) != '\0')
    {
        for (spanp = str2; (sc = *spanp++) != '\0';)
        {
            if (sc == c)
                break;
        }
        if (sc == '\0')
            return p - 1 - str1;
    }
    return p - str1;
}

char *strpbrk(const char *str1, const char *str2)
{
    const char *s1, *s2;

    for (s1 = str1; *s1 != '\0'; s1++)
    {
        for (s2 = str2; *s2 != '\0'; s2++)
        {
            if (*s1 == *s2)
                return (char *)s1;
        }
    }
    return NULL; // No matching character found
}

char *strtok_r(char *str, const char *delim, char **lasts)
{
    char *token;

    if (str == NULL)
        str = *lasts;
    str += strspn(str, delim); // Skip leading delimiters
    if (*str == '\0')
    {
        *lasts = str;
        return NULL;
    }

    token = str;
    str = strpbrk(token, delim);
    if (str == NULL)
    {
        *lasts = token + strlen(token);
    }
    else
    {
        *str = '\0';
        *lasts = str + 1;
    }

    return token;
}

char *strtok(char *str, const char *delim)
{
    static char *lasts;
    return strtok_r(str, delim, &lasts);
}

char *strcpy(char *dest, const char *src) {
    char *saved = dest;
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
    return saved;
}

char *strcat(char *dest, const char *src) {
    char *saved = dest;
    while (*dest) {
        dest++;
    }
    while ((*dest++ = *src++));
    return saved;
}
