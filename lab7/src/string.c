#include "string.h"
#include <stdint.h>
#include "mini_uart.h"

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

void *memcpy(void *dst, const void *src, size_t len)
{
        size_t i;

        /*
         * memcpy does not support overlapping buffers, so always do it
         * forwards. (Don't change this without adjusting memmove.)
         *
         * For speedy copying, optimize the common case where both pointers
         * and the length are word-aligned, and copy word-at-a-time instead
         * of byte-at-a-time. Otherwise, copy by bytes.
         *
         * The alignment logic below should be portable. We rely on
         * the compiler to be reasonably intelligent about optimizing
         * the divides and modulos out. Fortunately, it is.
         */

        if ((uintptr_t)dst % sizeof(long) == 0 &&
            (uintptr_t)src % sizeof(long) == 0 &&
            len % sizeof(long) == 0) {
                long *d = dst;
                const long *s = src;

                for (i=0; i<len/sizeof(long); i++) {
                        d[i] = s[i];
                }
        }
        else {
                char *d = dst;
                const char *s = src;

                for (i=0; i<len; i++) {
                        d[i] = s[i];
                }
        }

        return dst;
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

int strcpy(char *dst, const char *src)
{
    int ret = 0;

    while (*src) {
        *dst = *src;
        dst++;
        src++;
        ret++;
    }

    *dst = '\0';

    return ret;
}