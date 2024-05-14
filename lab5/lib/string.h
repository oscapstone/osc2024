#ifndef _STRING_H
#define _STRING_H

int strcmp(char *a, char *b);
char *strcpy(char *dst, const char *src);
char *strncpy(char *dst, const char *src, int n);
char *memcpy(char *dst, const char *src, int n);
int strlen(const char *s);
char* strtok(char* str, const char* delim);

#endif
