#include "include/utils.h"

int strcmp(const char* str1, const char* str2) {
    for (; (*str1 != '\0') || (*str2 != '\0'); str1++, str2++) {
        if (*str1 != *str2)
            return 0;
    }
    return 1;
}

// Convert integers to characters.
char int2str(int data) {
    switch (data) {
        case 10:
            return 'A';
        case 11:
            return 'B';
        case 12:
            return 'C';
        case 13:
            return 'D';
        case 14:
            return 'E';
        case 15:
            return 'F';
        default:
            // ascii code of '0' is 48.
            return data + 48;
    }
}