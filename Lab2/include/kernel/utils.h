#ifndef UTILS_H
#define UTILS_H
// Carriage Return '\r'
#define CR 13

char parse(const char c);

int string_len(const char* str);
// According to strcmp in string.h, return 0 if strings are the same
int string_comp(const char *str1, const char *str2);
// string for cpio or those not end with '\0' but with same length, this one requires length
int string_comp_l(const char *str1, const char *str2, int len);
// set a string to same value, it should under 127
void string_set(char *str, int n, int size);
// used for cpio length conversion(hex->int dec), it needs string length 
int h2i(const char *str, int len); 
// return a offset value to let i aligned to 'align' format
int align_offset(unsigned int i, unsigned int align);
// This one is for memory with 64bits
int align_mem_offset(void* i, unsigned int align);
// convert big endian to little endian 32bits
unsigned int BE2LE(unsigned int BE);

#endif