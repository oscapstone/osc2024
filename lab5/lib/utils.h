#ifndef _UTILS_H
#define _UTILS_H

unsigned int sprintf(char *dst, char *fmt, ...);
unsigned int vsprintf(char *dst, char *fmt, __builtin_va_list args);

int align4(int n);

#endif
