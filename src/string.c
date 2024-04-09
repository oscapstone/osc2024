#include "malloc.h"
#include "string.h"
#include "uart.h"

int strlen(const char* s)
{
    if (s == 0 || s[0] == '\0') {
        return 0;
    }

    // TODO: wrong length here
    int len = 0;
    while (s[len+1] != '\0') {
        len++;
    }
    return len;
}

int strcmp(const char* s1, const char* s2)
{
    int len1 = strlen(s1);
    int len2 = strlen(s2);

    if (len1 != len2) {
        return len1 - len2;
    }

    for (int i = 0; i < len1; i++) {
        if (s1[i] != s2[i]) {
            return s1[i] - s2[i];
        }
    }
    return 0;
}

/**
 * Convert char array to int.
 * Only works with number >= 0
 * 
 * Arguments:
 *      - s: char array
 *      - len: length of s
*/
int hex_atoi(const char *s, int len)
{
    char c;
    int num = 0;
    int n = 0;

    for (int i = 0; i < len; i++) {
        c = s[i];
        if (c >= '0' && c <= '9') {
            n = c - '0';
        } else if (c >= 'A' && c <= 'F') {
            n = c - 'A' + 10;
        } else if (c >= 'a' && c <= 'f') {
            n = c - 'a' + 10;
        }
        num = 16 * num + n;
    }

    return num;
}

/**
 * Getline from uart_getc(),
 * delimited by `delim`.
 * 
 * params:
 *  - lineptr: pointer to char array (char *)
 *  - n: size of char array
 *  - delim: delimiter
*/
int getdelim(char **lineptr, int n, int delim)
{
    *lineptr = (char *) malloc(sizeof(char) * n);

    char c;
    int idx = 0;
    while ((c = (char) uart_getc()) != delim) {
        (*lineptr)[idx++] = c;
        uart_send(c);
    }
    (*lineptr)[idx++] = '\0';
    uart_send('\n');

    return idx;
}

/**
 * Getline from uart_getc()
*/
int getline(char **lineptr, int n)
{
    return getdelim(lineptr, n, '\n');
}
