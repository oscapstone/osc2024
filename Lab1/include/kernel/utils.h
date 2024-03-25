#ifndef UTILS_H
#define UTILS_H
// Carriage Return '\r'
#define CR 13

char parse(const char c);

int string_len(const char* str);
// According to strcmp in string.h, return 0 if strings are the same
int string_comp(const char *str1, const char *str2);
// set a string to same value, it should under 127
void string_set(char *str, int n, int size);

#endif