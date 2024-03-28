#ifndef _UTIL_H
#define _UTIL_H

#include "type.h"

int strcmp(char* str1, char* str2);
int strncmp(char* str1, char* str2, uint32_t len);
void strncpy(char* src, char* dst, uint32_t len);
uint32_t strlen(char* str);
int atoi(char* val_str, int len);

uint32_t to_little_endian(uint32_t val);

#endif