#ifndef _STRING_H_
#define _STRING_H_

#include "stdint.h"

size_t strlen(const char *str);
int strcmp(const char *p1, const char *p2);
int strncmp(const char *s1, const char *s2, size_t n);
char *strcpy(char *dest, const char *src);
void *memcpy(void *dest, const void *src, size_t count);
void *memset(void *s, int c, size_t n);
int atoi(char* str);

#endif /* _STRING_H_ */