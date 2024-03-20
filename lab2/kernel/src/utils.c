#include "../include/utils.h"
#include <stddef.h>

// this function writes formatted string into reg, and returns its size
unsigned int vsprintf(char* dst, char* fmt, __builtin_va_list args)
{
	// dst: input args buffer
	// fmt: input formatted string
	long int arg;
	int len, sign, i;
	char *p, *orig = dst, tmpstr[19];

	// failsafes
	if (dst == (void*)0 || fmt == (void*)0) {
		return 0;
	}

	// main loop
	arg = 0;
	while (*fmt) {
		if (dst - orig > VSPRINT_MAX_BUF_SIZE - 0x10) {
			return -1;
		}

		// argument access
		if (*fmt == '%') {
			fmt++;
			// case '%%'
			if (*fmt == '%') {
				goto put;
			}

			len = 0; // size modifier (precision)
			while (*fmt >= '0' && *fmt <= '9') {
				len *= 10;
				len += *fmt - '0';
				fmt++;
			}

			if (*fmt == 'l') { // long
				fmt++;
			}

			if (*fmt == 'c') { // char
				arg = __builtin_va_arg(args, int);
				*dst++ = (char)arg;
				fmt++;
				continue;
			}

			else if (*fmt == 'd') { // decimal
				arg = __builtin_va_arg(args, int);
				sign = 0;
				if ((int)arg < 0) {
					arg *= -1;
					sign = 1; // #
				}
				if (arg > 99999999999999999L) {
					arg = 99999999999999999L;
				}

				// convert to string
				i = 18;
				tmpstr[i] = 0;
				do {
					tmpstr[--i] = '0' + (arg % 10);
					arg /= 10;
				} while (arg != 0 && i > 0);
				if (sign) {
					tmpstr[--i] = '-';
				}
				if (len > 0 && len < 18) { // size modifier
					while (i > 18 - len) {
						tmpstr[--i] = ' ';
					}
				}

				p = &tmpstr[i]; // p = head of output string
				goto copystring;
			} else if (*fmt == 'x') {
				arg = __builtin_va_arg(args, long int);
				i = 16;
				tmpstr[i] = 0;
				do {
					char n = arg & 0xf;
					tmpstr[--i] = n + (n > 9 ? 0x37 : 0x30);
					arg >>= 4;
				} while (arg != 0 && i > 0);
				if (len > 0 && len <= 16) { // size modifier
					while (i > 16 - len) {
						tmpstr[--i] = '0';
					}
				}
				p = &tmpstr[i];
				goto copystring;
			} else if (*fmt == 's') {
				p = __builtin_va_arg(args, char*);
			copystring:
				if (p == (void*)0) {
					p = "(null)";
				}
				while (*p) {
					*dst++ = *p++; // copy p's content into dst
				}
			}
		} else {
		put:
			*dst++ = *fmt;
		}
		fmt++;
	}
	*dst = 0; // add '0' at last pos to end string
	return dst - orig;
}

unsigned int sprintf(char* dst, char* fmt, ...)
{
	__builtin_va_list args;        // pack '...' variables
	__builtin_va_start(args, fmt); // store them from fmt
	unsigned int r = vsprintf(dst, fmt, args);
	__builtin_va_end(args);
	return r;
}

int strcmp(const char* p1, const char* p2)
{
	const unsigned char* s1 = (const unsigned char*)p1;
	const unsigned char* s2 = (const unsigned char*)p2;
	unsigned char c1, c2;

	do {
		c1 = (unsigned char)*s1++;
		c2 = (unsigned char)*s2++;
		if (c1 == '\0')
			return c1 - c2;
	} while (c1 == c2);
	return c1 - c2;
}

unsigned long long strlen(const char* str)
{
	size_t count = 0;
	// stop when encounter '\0'
	while ((unsigned char)*str++) {
		count++;
	}
	return count;
}

int strncmp(const char* s1, const char* s2, unsigned long long n)
{
	unsigned char c1 = '\0';
	unsigned char c2 = '\0';
	if (n >= 4) {
		size_t n4 = n >> 2;
		do {
			// loop unroll
			c1 = (unsigned char)*s1++;
			c2 = (unsigned char)*s2++;
			if (c1 == '\0' || c2 == '\0')
				return c1 - c2;
			c1 = (unsigned char)*s1++;
			c2 = (unsigned char)*s2++;
			if (c1 == '\0' || c2 == '\0')
				return c1 - c2;
			c1 = (unsigned char)*s1++;
			c2 = (unsigned char)*s2++;
			if (c1 == '\0' || c2 == '\0')
				return c1 - c2;
			c1 = (unsigned char)*s1++;
			c2 = (unsigned char)*s2++;
			if (c1 == '\0' || c2 == '\0')
				return c1 - c2;
		} while (--n4 > 0);

		n &= 3;
	}
	while (n > 0) {
		c1 = (unsigned char)*s1++;
		c2 = (unsigned char)*s2++;
		if (c1 == '\0' || c1 != c2)
			return c1 - c2;
		n--;
	}
	return c1 - c2;
}

char* memcpy(void* dest, const void* src, unsigned long long len)
{
	char* d = dest;
	const char* s = src;
	while (len--) {
		*d++ = *s++;
	}
	return dest;
}