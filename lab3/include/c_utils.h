#ifndef _C_UTILS_H
#define _C_UTILS_H 

int strcmp(const char *str1, const char *str2);
int strncmp(const char *str1, const char *str2, int len);
char *strncpy(char *dest, const char *src, int len);
int strlen(const char *str);
int memcmp(const void *str1, const void *str2, int len);
unsigned int is_visible(unsigned int c);
int hextoi(const char *s, int len);
int atoi(const char *s);
unsigned int endian_big2little(unsigned int x);

#endif