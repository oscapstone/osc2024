#ifndef __STRING_H__
#define __STRING_H__

#include "stdint.h"

#define ENDL "\r\n"

int strcmp(const char *a, const char *b) {
    uint32_t i = 0;
    while (a[i] == b[i] && a[i] != '\0' && b[i] != '\0') i++;
    return a[i] - b[i];
}

uint32_t strlen(const char *a) {
    for (uint32_t i = 0;; i++)
        if (a[i] == '\0') return i;
    return 0;
}

#endif