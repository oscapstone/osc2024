#include"header/utils.h"
#include"header/uart.h"

int string_compare(char *s1,char *s2) {
    while (*s1 && *s2 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return !(*s1 - *s2);
}

int strncmp(char *s1, char *s2, int n) {
    while (n-- && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    if (n == (int) -1) {
        return 0;
    }
    return *(const unsigned char *) s1 - *(const unsigned char *) s2;
}
int isdigit(int c){
    return c >= '0' && c <= '9';
}
int toupper(int c){
    if (c >= 'a' && c <= 'z') {
        return c - 'a' + 'A';
    } else {
        return c;
    }
}

int ishex(int c){
    return isdigit(c) || (toupper(c) >= 'A' && toupper(c) <= 'F');
}

unsigned int hex_to_int(char *a, int size){
    unsigned int result = 0;

    for (int i = 0; i < size; i++) {
        char c = a[i];
        if (ishex(c)) {
            int val = isdigit(c) ? c - '0' : toupper(c) - 'A' + 10;
            result = (result << 4) + val;
        }
    }

    return result;
}

char* strtok(char* str, const char* delimiters) {
    static char* buffer = 0;
    if (str != 0) {
        buffer = str;
    }
    if (buffer == 0) {
        return 0;
    }
    char* start = buffer;
    while (*buffer != '\0') {
        const char* delim = delimiters;
        while (*delim != '\0') {
            if (*buffer == *delim) {
                *buffer = '\0';
                buffer++;
                if (start != buffer) {
                    return start;
                } else {
                    start++;
                    break;
                }
            }
            delim++;
        }
        if (*delim == '\0') {
            buffer++;
        }
    }
    if (start == buffer) {
        return 0;
    } else {
        return start;
    }
}
char *strcpy(char *dest, const char *src) {
    char *ret = dest;
    while ((*dest++ = *src++));
    return ret;
}

unsigned int strlen(const char *s) {
    int len = 0;
    while (*s++) len++;
    return len;
}