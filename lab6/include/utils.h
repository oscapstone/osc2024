#pragma once

/**
 * @brief Align `n` to be a multiple of 4.
 *
 * @param n A number
 * @return Algined number
 */
int align4(int n);

int atoi(const char *s);

/**
 * @brief Convert hexadecimal string to int.
 *
 * @param s: hexadecimal string
 * @param n: string length
 * @return Converted int number
 */
int hextoi(char *s, int n);

int memcmp(const void *str1, const void *str2, int n);
void *memcpy(void *dest, const void *src, int n);
void *memset(void *s, int c, int n);