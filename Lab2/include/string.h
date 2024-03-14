#ifndef __STRING_H__
#define __STRING_H__

int strlen(const char* str);
int strcmp(const char* str1, const char* str2);
int memcmp(const void *s1, const void *s2, int n);
int hstr2int(const char* str, int len);

#endif