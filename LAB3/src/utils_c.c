#include "utils_c.h"
#include "mini_uart.h"
#include <stddef.h>

/*
    string part
*/

int utils_str_compare(const char *a,const char *b)
{
    char aa, bb;
    do
    {
        aa = (char)*a++;
        bb = (char)*b++;
        if (aa == '\0' || bb == '\0')
        {
            return aa - bb;
        }
    } while (aa == bb);
    return aa - bb;
}
void utils_newline2end(char *str)
{
    while (*str != '\0')
    {
        if (*str == '\n')
        {
            *str = '\0';
            return;
        }
        ++str;
    }
}

void utils_int2str_dec(int num, char *str)
{
    // num=7114 digit=4
    int digit = -1, temp = num;
    while (temp > 0)
    {
        temp /= 10;
        digit++;
    }
    for (int i = digit; i >= 0; i--)
    {
        int t = 1;
        for (int j = 0; j < i; j++)
        {
            t *= 10;
        }
        *str = '0' + num / t;
        num = num % t;
        str++;
    }
    *str = '\0';
}

void utils_uint2str_dec(unsigned int num, char *str)
{
    char *start = str;
    if(num == 0){
        *str++ = '0';
        *str = '\0';
        return;
    }

    while(num > 0){
        *str++ = (num%10) + '0';
        num /= 10;
    }

    *str = '\0';

    char *end = str - 1;
    char tmp;
    while(start < end){
        tmp = *start;
        *start = *end;
        *end = tmp;
        start++;
        end--;
    }
}

void cat_two_strings(char *str1, char *str2, char *new_str){

    char *ptr = new_str;

    while (*str1 != '\0') {
        *ptr++ = *str1++;
    }
    while (*str2 != '\0') {
        *ptr++ = *str2++;
    }

    *ptr = '\0';
}

void mini_printf(const char *format, const char *str) {
    while (*format) {
        if (*format == '%' && *(format + 1) == 's') {
            uart_send_string(str);  // Assuming uart_send_string sends strings
            format += 2;  // Skip the format specifier
        } else {
            uart_send(*format);  // Assuming uart_send_char sends individual characters
            format++;
        }
    }
}

unsigned int utils_str2uint_dec(const char *str)
{
    unsigned int value = 0u;

    while (*str)
    {
        value = value * 10u + (*str - '0');
        ++str;
    }
    return value;
}

size_t utils_strlen(const char *s) {
  size_t i = 0;
  while (s[i]) i++;
  return i+1;
}

/*
    reboot part
*/

void set(long addr, unsigned int value)
{
    volatile unsigned int *point = (unsigned int *)addr;
    *point = value;
}

void reset(int tick)
{                                     // reboot after watchdog timer expire
    set(PM_RSTC, PM_PASSWORD | 0x20); // full reset
    set(PM_WDOG, PM_PASSWORD | tick); // number of watchdog tick
}

void cancel_reset()
{
    set(PM_RSTC, PM_PASSWORD | 0); // full reset
    set(PM_WDOG, PM_PASSWORD | 0); // number of watchdog tick
}

/*
    others
*/

void align(void *size, size_t s) 
{
    unsigned int *x = (unsigned int *)size;
    if ((*x) & (s-1))
    {
        (*x) += s - ((*x) & (s-1));
    }
}

uint32_t align_up(uint32_t size, int alignment) {
  return (size + alignment - 1) & -alignment;
}