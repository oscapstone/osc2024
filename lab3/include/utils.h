#ifndef UTILS_H
#define UTILS_H

int strcmp(const char *s1, const char *s2);
void str_int_2_decimal(char *buf, int value);
void str_uint_2_decimal(char *buf, unsigned int value);
void str_ulong_2_decimal(char *buf, unsigned long value);
void str_num_2_decimal(char *buf, unsigned long value, int is_negative);
void str_ulong_2_hex(char *buf, unsigned long value);
void str_reverse(char *start, char *end);
void vsnprintf(char *buf, unsigned int size, const char *fmt,
               __builtin_va_list args);
int strncmp(const char *s1, const char *s2, unsigned int n);
unsigned int strlen(const char *str);
char *strtok(char *str, const char *delimiters, char **saveptr);
char *strchr(const char *str, int c);
int atoi(const char *str);
void strcpy(char *dest, const char *src);
void strcat(char *dest, const char *src);

#endif /* UTILS_H */
