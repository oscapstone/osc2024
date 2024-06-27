#ifndef _STRING_H
#define _STRING_H

#include <lib/stddef.h>

int strcmp(const char *a, const char *b);
char *strcat(char *dest, const char *src);
char *strcpy(char *dst, const char *src);
char *strncpy(char *dst, const char *src, int n);
void *memcpy(void *dest, const void *src, int n);
void *memset(void *str, int c, size_t n);
int strlen(const char *s);
char* strtok(char* str, const char* delim);

#endif
