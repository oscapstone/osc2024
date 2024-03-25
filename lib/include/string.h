#ifndef _STRING_H
#define _STRING_H

#include "int.h"

/**
 * compare part of two string
 * @return 0 if the same, -1 otherwise
 */
int strncmp(const char *str1, const char *str2, size_t size);

/**
 * convert an integer value into a string
 */
char *itoa(int value, char *s);

#endif // _STRING_H
