#include "string.h"

int strcmp(char *a, char *b) {
    while (*a && (*a == *b)) {
        a++, b++;
    }
    return *(const unsigned char *)a - *(const unsigned char *)b;
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

int strlen(const char *str) {
    int len = 0;
    while (*str) {
        str++;
        len++;
    }
    return len;
}
