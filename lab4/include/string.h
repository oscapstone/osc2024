#ifndef _STRING_H_
#define _STRING_H_

unsigned int sprintf(char *dst, char *fmt, ...);
unsigned int vsprintf(char *dst, char *fmt, __builtin_va_list args);

int strlen(const char *s);
int strcmp(const char *cs, const char *ct);
int strncmp(const char *cs, const char *ct, int count);
char *strcpy(char *dest, const char *src);
char *strtok(char *str, const char *delimiters);

// int isalpha(char c);
// int isdigit(int c);
// int toupper(int c);
// int isxdigit(int c);
unsigned int parse_hex_str(char *s, unsigned int max_len);
long long atoi(const char *s);

#endif