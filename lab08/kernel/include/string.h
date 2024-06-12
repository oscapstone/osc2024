#ifndef __STRING_H__
#define __STRING_H__

#include "type.h"

int strcmp(const char *str1, const char *str2);
char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, int n);
uint32_t strlen(const char* str);
char *strtok(char* str, const char* delim);

#endif