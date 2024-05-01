#include "stdint.h"

size_t strlen(const char *str);
int strcmp(const char *p1, const char *p2);
int strncmp(const char *s1, const char *s2, size_t n);
char *strcpy(char *dest, const char *src);
int atoi(char* str);
void *memcpy(void *dest, const void *src, size_t count);