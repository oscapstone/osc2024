#ifndef _UTILS_H_
#define _UTILS_H_

// Power
#define PM_PASSWORD 0x5A000000
#define PM_RSTC     0x3F10001C
#define PM_WDOG     0x3F100024

int strlen(const char *s);
int strcmp(const char *a, const char *b); /* return 0 if a == b */
int strncmp(const char *s1, const char *s2, unsigned long long n);
unsigned int hex_to_dec(char *s);
unsigned int hex_to_decn(char *s, unsigned int len);

#endif