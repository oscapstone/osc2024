#ifndef _UTILS_H_
#define _UTILS_H_

#include <stddef.h>

int strcmp(const char* str1, const char* str2);
char* uint_to_string(unsigned int data);
char int2char(int data);
void delay(unsigned int time);
unsigned int power(unsigned int base, int p);
unsigned int strHex2Int(char* str, int len);
char* strcpy(char* destination, const char* source);
long unsigned int strlen(const char* str);

#endif