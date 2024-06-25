#ifndef _UTILS_H_
#define _UTILS_H_

#include "stdint.h"

#define VSPRINT_MAX_BUF_SIZE 128

unsigned int sprintf(char *dst, char* fmt, ...);
unsigned int vsprintf(char *dst,char* fmt, __builtin_va_list args);

unsigned long long  strlen(const char *str);
int                 strcmp(const char*, const char*);
int                 strncmp(const char*, const char*, unsigned long long);
void                *memcpy(void *dest, const void *src, size_t n);
void                *memset(void *s, int c, size_t n);
void                str_split(char* str, char delim, char** result, int* count);
char*               strcpy(char *dest, const char *src);
int                 atoi(const char* str);
char                async_getchar();
void                async_putchar(char c);
void                puts(char *s);
void                put_int(int num);
void                put_hex(unsigned int num);
void                printf(char *fmt, ...);
char                getchar();
void                putchar(char c);

#endif /* _UTILS_H_ */
