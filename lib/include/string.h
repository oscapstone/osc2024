#ifndef _STRING_H
#define _STRING_H

#include "int.h"

/**
 * tokenize string that separated by delimeter
 * @params
 * - n: size of dest buffer
 * @return the next address of the token
 */
char *strtok(char *src, char *dest, size_t n, char delimeter);

/**
 * compare part of two string
 * @return 0 if the same, -1 otherwise
 */
int strncmp(const char *str1, const char *str2, size_t size);

/**
 * compute the length of string
 * @return n if can't find '\0' in the string
 */
u32_t strnlen(char *s, u32_t n);

/**
 * convert an integer value into a string
 */
char *itoa(int value, char *s);

/**
 * check if the given character is whitespace character
 * @return 1 if the character is a whitespace character, 0 otherwise
 */
int isspace(char c);

char tolower(char c);
u32_t hex_to_i(char *h, int n);

#endif  // _STRING_H
