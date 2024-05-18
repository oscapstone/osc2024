#ifndef STRING_H
#define STRING_H

int strcmp(const char *str1, const char *str2);
int memcmp(const void *str1, const void *str2, int n);
char *strncpy(char *dest, const char *src, int n);
int strlen(const char *str);
char *strcat(char *dest, const char *src);
void *memcpy(void *dest, const void *src, int n);
void *memset(void *s, int c, int n);

#endif // STRING_H