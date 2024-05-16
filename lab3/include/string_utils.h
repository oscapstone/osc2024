#ifndef	STRING_UTILS_H
#define	STRING_UTILS_H

#include <stdint.h>

int my_strcmp(char *s1, char *s2);
unsigned char hexchar2val(char ch);
unsigned int hexstr2val(char *str, unsigned int size);
unsigned int my_strlen(char *str);
void my_strncpy(char *dest, char *src, uint32_t n);
unsigned int my_stoi(const char *str);

#endif  