#ifndef UTILS_H
#define UTILS_H

int strcmp(const char *s1, const char *s2);
void int_to_decimal(char *buf, int value);
void int_to_hex(char *buf, unsigned int value);
void my_vsprintf(char *buf, const char *fmt, __builtin_va_list args);

#endif
