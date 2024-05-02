#ifndef _STRING_H_
#define _STRING_H_

#include "stddef.h"
#define VSPRINT_MAX_BUF_SIZE 0x100

unsigned int sprintf(char *dst, char* fmt, ...);
unsigned int vsprintf(char *dst,char* fmt, __builtin_va_list args);

unsigned long long strlen(const char *str);
int                strcmp(const char*, const char*);
int                strncmp(const char*, const char*, unsigned long long);
char*              memcpy(void *dest, const void *src, unsigned long long len);
char*              strcpy(char *dest, const char *src);
void*              memset(void *s, int c, size_t n);
char*              strchr(register const char *s, int c);

int str_SepbySpace(char* head, char* words[]);
int   atoi(char* str);

#endif /* _STRING_H_ */
