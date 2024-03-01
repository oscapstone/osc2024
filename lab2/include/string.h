#ifndef STRING_H
#define STRING_H

int strcmp(const char *str1, const char *str2);
int memcmp(const void *str1, const void *str2, int n);
char *strncpy(char *dest, const char *src, int n);

#endif // STRING_H