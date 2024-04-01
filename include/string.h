#ifndef _STRING_H
#define _STRING_H

int strcmp(const char *X, const char *Y);
int strncmp(const char *X, const char *Y, unsigned int n);
unsigned int strlen(const char *s);
unsigned int atoi(char *str);
char *memcpy(void *dest, const void *src, unsigned long long len);
#endif