#ifndef __STRING_H__
#define __STRING_H__

#include "stdint.h"

#define ENDL "\r\n"

int strcmp(const char *a, const char *b);
int strncmp(const char *a, const char *b, uint32_t size);
uint32_t strlen(const char *a);
uint32_t hex_ascii_to_uint32(const char *str, uint32_t size);
uint32_t get_be_uint32(void *ptr);


#endif