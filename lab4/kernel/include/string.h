#include "stddef.h"
#define VSPRINT_MAX_BUF_SIZE 0x100

size_t strlen(const char *str);
int strcmp(const char *p1, const char *p2);
int strncmp(const char *s1, const char *s2, unsigned long long n);
char *strcpy(char *dest, const char *src);
unsigned int vsprintf(char *dst, char* fmt, __builtin_va_list args);
