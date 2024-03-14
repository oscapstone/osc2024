#include "mini_uart.h"


int str_cmp(const char *s1, const char *s2)
{
    const unsigned char *p1 = (const unsigned char *) s1;
    const unsigned char *p2 = (const unsigned char *) s2;

    p1 = (const unsigned char *) s1;
    p2 = (const unsigned char *) s2;

    while (*p1 && *p1 == *p2) ++p1, ++p2;

    return (*p1 > *p2) - (*p2  > *p1);
}


int str_eql(const char *s1, const char *s2)
{
    return str_cmp(s1, s2) == 0;
}


void reverse(char *str, int length)
{
    int start = 0;
    int end = length - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        end--;
        start++;
    }
}


void uint_to_hex(unsigned int n, char *buffer) 
{
    int length = 0;
    while (n != 0) {
        int rem = n % 16;
        buffer[length++] = (rem < 10) ? rem + '0' : rem + 'A';
        n = n / 16;
    }
    if (length == 0) buffer[length++] = '0';
    buffer[length] = '\0';
    reverse(buffer, length);
}


void uint_to_ascii(unsigned int n, char *buffer)
{
    int length = 0;   
    while (n != 0) {
        int rem = n % 10;
        buffer[length++] = rem + '0';
        n = n / 10;
    }
    if (length == 0) buffer[length++] = '0';
    buffer[length] = '\0';
    reverse(buffer, length);
}