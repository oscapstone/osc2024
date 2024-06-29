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