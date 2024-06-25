#ifndef STRING_H
#define STRING_H

int oct2bin(char *s, int n);
int hex2bin(char *s, int n);
int strcmp(const char *a, const char *b);
int memcmp(void *s1, void *s2, int n);
int strlen(const char *str);
int strcpy(char *dst, const char *src);
int strcat(char *dst, const char *src);
int atoi(char *s);

void *memset(void *str, int c, unsigned long n);
void *memcpy(void *dest, const void *src, unsigned long n);

#endif