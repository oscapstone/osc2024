#ifndef UTILS_H
#define UTILS_H

int strcmp(const char *s1, const char *s2);
void int_to_decimal(char *buf, int value);
void uint_to_decimal(char *buf, unsigned int value);
void ulong_to_decimal(char *buf, unsigned long value);
void ull_to_hex(char *buf, unsigned long long value);
void my_vsprintf(char *buf, const char *fmt, __builtin_va_list args);
int strncmp(const char *s1, const char *s2, unsigned int n);
unsigned int strlen(const char *str);

#define NULL (void *)0

#endif
