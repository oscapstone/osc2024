#include <stdint.h>
// #include <stdlib.h>
// #include <stddef.h>

#ifdef _cplusplus
#define NULL 0
#else
#define NULL (void*)0
#endif

int ut_string_compare(const char* i, const char* j);
unsigned long ut_atoi(const char *s, int char_size);
void ut_align(void *size, unsigned int s);
uint32_t ut_align_up(uint32_t size, int alignment);
uint32_t ut_strlen(const char *s);