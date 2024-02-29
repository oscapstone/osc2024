#ifndef _MY_STRING_H
#define _MY_STRING_H

char *itoa(int value, char *s);
unsigned int vsprintf(char *dst, char *fmt, __builtin_va_list args);
unsigned int sprintf(char *dst, char *fmt, ...);
int strcmp(const char *X, const char *Y);

#endif