#ifndef _STRING_H
#define _STRING_H


unsigned int is_visible(unsigned int c);
int strcmp(const char* str1, const char* str2);
int memcmp(const void *str1, const void *str2, int n);
char *strncpy_(char *dest, const char *src, int n);
int strlen(const char *str);
int strncmp(const char *s1, const char *s2, int n);

#endif // _STRING_H