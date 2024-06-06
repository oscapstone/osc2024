#ifndef __STRING_H__
#define __STRING_H__

int strlen(const char* str);
int strcmp(const char* str1, const char* str2);
int strcpy(char* str1, const char* str2);
char *strdup(const char* str);
char *strtok(char* str, const char* delimeter);
int memcmp(const void *s1, const void *s2, int n);
void memset(void* ptr, int value, int n);
void memcpy(void* ptr1, void *ptr2, int n);
int hstr2int(const char* str, int len);
int atoi(char *str);
int gets(char *str);

#endif