#include <stdint.h>
#include <stddef.h>

int utils_string_compare(const char* str1,const char* str2);
unsigned long utils_hex2dec(const char *s, int char_size);
void utils_align(void *size, unsigned int s);
uint32_t utils_align_up(uint32_t size, int alignment);
size_t utils_strlen(const char *s);
int utils_keyword_compare(const char* str1,const char* str2);