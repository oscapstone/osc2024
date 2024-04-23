#ifndef __LIB_H__
#define __LIB_H__

#include "type.h"

uint32_t strtol(const char *sptr, uint32_t base, int size);
uint64_t atoi(const char *str);
uint64_t pow(int base, int exp);
void assert(int condition, char* message);

#endif