#ifndef _STRING_H
#define _STRING_H

#include "types.h"

int strcmp(const char *X, const char *Y);
int strncmp(const char *X, const char *Y, uint32_t n);
uint32_t strlen(const char *s);
uint32_t atoi(char *str);
char *memcpy(void *dest, const void *src, uint64_t len);
#endif