#ifndef STRING_H
#define STRING_H

#include "def.h"

unsigned int str_len(const char* str);
int str_cmp(const char* a, const char* b);
int str_n_cmp(const char* a, const char* b, unsigned int n);
unsigned int hexstr2int(char* hex_str);
int decstr2int(char* dec_str);
char* str_tok(char* str, const char* delim);
char* str_cpy(char* dest, const char* src);
char* str_n_cpy(char* dest, const char* src, unsigned int n);

#endif /* STRING_H */
