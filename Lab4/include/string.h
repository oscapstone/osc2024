#ifndef _STRING_H_
#define _STRING_H_

#define VSPRINT_MAX_BUF_SIZE 0x100

char* str_arg(char* str);
int strcmp(const char* p1, const char* p2);
char * strcat(char * s, const char * append);
int strncmp (const char *s1, const char *s2, unsigned long long n);
unsigned int vsprintf(char *dst, char* fmt, __builtin_va_list args);
unsigned int sprintf(char *dst, char* fmt, ...);
unsigned long long strlen(const char *str);
char* memcpy(void *dest, const void *src, unsigned long long len);
char* strcpy (char *dest, const char *src);
int atoi(char* str);

#endif /* _STRING_H_ */