#include "string.h"
#include "uart.h"

int strlen(const char* s)
{
    if (s == 0 || s[0] == '\0') {
        return 0;
    }

    int len = 0;
    while (s[len+1] != '\0') {
        len++;
    }
    return len;
}

int strcmp(const char* s1, const char* s2)
{
    int len1 = strlen(s1);
    int len2 = strlen(s2);

    if (len1 != len2) {
        return len1 - len2;
    }

    for (int i = 0; i < len1; i++) {
        if (s1[i] != s2[i]) {
            return s1[i] - s2[i];
        }
    }
    return 0;
}
