#include "utils.h"

int strcmp(const char* str1, const char* str2) {
    for (; *str1 == *str2; str1++, str2++) {
        if (*str1 == '\0')
            return 0;
    }
    return (unsigned char)*str1 - (unsigned char)*str2;
}

char* strcpy(char* destination, const char* source) {
    int ind = 0;
    for (; source[ind] != '\0'; ind++) {
        destination[ind] = source[ind];
    }
    destination[ind] = '\0';

    return destination;
}

// Convert integers to characters.
char int2char(int data) {
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

// Convert hexadecimal strings to unsigned integers. Pass in the string array and the length of
// the string.
unsigned int strHex2Int(char* str, int len) {
    unsigned int tot = 0;

    for (int i = 0; i < len; i++) {
        switch (str[i]) {
            case 'A':
                tot += 10 * power(16, len - i - 1);
                break;
            case 'B':
                tot += 11 * power(16, len - i - 1);
                break;
            case 'C':
                tot += 12 * power(16, len - i - 1);
                break;
            case 'D':
                tot += 13 * power(16, len - i - 1);
                break;
            case 'E':
                tot += 14 * power(16, len - i - 1);
                break;
            case 'F':
                tot += 15 * power(16, len - i - 1);
                break;
            default:
                // Ascii code for '0' is 48.
                tot += (str[i] - 48) * power(16, len - i - 1);
        }
    }

    return tot;
}

// Calculating base to the power of p.
unsigned int power(unsigned int base, int p) {
    unsigned int tot = 1;
    for (int i = 0; i < p; i++) {
        tot *= base;
    }
    return tot;
}

long unsigned int strlen(const char* str) {
    int i = 0;
    while (str[i] != '\0') {
        i++;
    }
    
    return i;
}

void delay(unsigned int clock) {
    while (clock--) {
        asm volatile (
            "nop;"
        );
    }
}