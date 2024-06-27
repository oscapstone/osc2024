#include <lib/stdlib.h>
#include <lib/string.h>

int strcmp(const char *a, const char *b) {
    while (*a && (*a == *b)) {
        a++, b++;
    }
    return *(const unsigned char *)a - *(const unsigned char *)b;
}

char* strcat (char *dest, const char *src)
{
  strcpy (dest + strlen (dest), src);
  return dest;
}

char *strcpy(char *dest, const char *src) {
    char *start = dest;

    while (*src != '\0') {
        *dest = *src;
        dest++;
        src++;
    }

    *dest = '\0';
    return start;
}

char *strncpy(char *dest, const char *src, int n) {
    char *start = dest;
    while (*src != '\0' && n > 0) {
        *dest = *src;
        dest++;
        src++;
        n--;
    }
    *dest = '\0';
    return start;
}

void *memcpy(void *dest, const void *src, int n)
{
    char *d = dest;
    const char *s = src;
    while (n--)
        *d++ = *s++;
    return dest;
}

void *memset(void *s, int c, size_t n) {
    unsigned char *p = (unsigned char *)s;
    while (n--) {
        *p++ = (unsigned char)c;
    }
    return s;
}

int strlen(const char *str) {
    int len = 0;
    while (*str) {
        str++;
        len++;
    }
    return len;
}

char *strtok(char *s, const char *delim) {
    char *spanp;
    int c, sc;
    char *tok;
    static char *last;

    if (s == NULL && (s = last) == NULL) {
        return NULL;
    }

    c = *s++;
    for (spanp = (char *)delim; (sc = *spanp++);) {
        if (c == sc) {
            c = *s++;
            spanp = (char *)delim;
        }
    }

    if (c == 0) {
        last = NULL;
        return (NULL);
    }
    tok = s - 1;

    while (1) {
        c = *s++;
        spanp = (char *)delim;
        do {
            if ((sc = *spanp++) == c) {
                if (c == 0) {
                    s = NULL;
                } else {
                    s[-1] = 0;
                }
                last = s;
                return (tok);
            }
        } while (sc != 0);
    }
}
