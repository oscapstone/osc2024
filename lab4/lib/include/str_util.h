#ifndef __STR_UTIL_H
#define __STR_UTIL_H

int str_len(char* str);
int str_cmp(const char *p1, const char *p2);
int str_cmp_len(const char *p1, const char *p2, unsigned int len);
void str_reverse(char* str, int len);

int str_to_int(char *str);
int int_to_str(int num, char* str, int base);
int hexstr_to_int(char* str);

unsigned int swap_endian(unsigned int b_end);

void *memset(void* ptr, int val, unsigned int len);

#endif