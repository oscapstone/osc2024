#ifndef MY_STRING_H
#define MY_STRING_H

#include "../include/my_stddef.h"

#define HEX_DIGIT_TO_INT(c) ((c >= '0' && c <= '9') ? (c - '0') : ((c >= 'a' && c <= 'f') ? (c - 'a' + 10) : (c - 'A' + 10)))

void strset( char * s1, int c, int size );
int  strlen(const char * s );
int strncmp(const char *s1, const char *s2, size_t n);
int strcmp(const char *s1, const char *s2);
void itoa(int num, char *str, int base);
unsigned int hex_to_uint(char *hex);
char *strrchr(const char *str, int ch);
unsigned long long strtoull(const char *str, char **endptr, int base);
char *strtok(char *str, const char *delim);
char *strcpy(char *dest, const char *src);
char *strchr(const char *str, int c);
int atoi(const char *str);

#endif
