#include "string_utils.h"

int my_strcmp(char *s1, char *s2)
{
    while (*s1 == *s2) {
        s1++;
        s2++;
        if (*s1 == '\0')
            break;
    }
    return *s1 - *s2;
}