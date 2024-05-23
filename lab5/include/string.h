#ifndef _STRING_H
#define _STRING_H

#include <stddef.h>

unsigned int is_visible(unsigned int c);
int strcmp(const char* str1, const char* str2);
int memcmp(const void *str1, const void *str2, int n);
void *memcpy(void *dst, const void *src, size_t len);
char *strncpy_(char *dest, const char *src, int n);
int strlen(const char *str);
int strncmp(const char *s1, const char *s2, int n);

#endif // _STRING_H