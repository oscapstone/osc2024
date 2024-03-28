#include "string.h"

unsigned int is_visible(unsigned int c){
    if(c >= 32 && c <= 126){
        return 1;
    }
    return 0;
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && *s2 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *s1 - *s2;
}


int memcmp(const void *str1, const void *str2, int n) {
	const unsigned char *a = str1, *b = str2;
	while (n-- > 0) {
		if (*a != *b) {
			return *a - *b;
		}
		a++;
		b++;
	}
	return 0;
}

char *strncpy_(char *dest, const char *src, int n) {
	while (n-- && (*dest++ = *src++))
		;
	return dest;
}

int strlen(const char *str) {
	int len = 0;
	while (*str++ != '\0')
		len++;
	return len;
}

int strncmp(const char *s1, const char *s2, int n) {
    while (n-- && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    if (n == (int) -1) {
        return 0;
    }
    return *(const unsigned char *) s1 - *(const unsigned char *) s2;
}