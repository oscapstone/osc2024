#include "utils.h"
#include <stddef.h>

unsigned int vsprintf(char *dst, char *fmt, __builtin_va_list args)
{
    long int arg;
    int len, sign, i;
    char *p, *orig = dst, tmpstr[19];

    // Check if the destination or format strings are NULL
    if (dst == (void *)0 || fmt == (void *)0)
    {
        return 0;
    }

    // Main loop to process the format string
    arg = 0;
    while (*fmt)
    {
        // Check for buffer overflow
        if (dst - orig > VSPRINT_MAX_BUF_SIZE - 0x10)
        {
            return -1;
        }
        // Check for format specifier
        if (*fmt == '%')
        {
            fmt++;
            // Check for escaped percent sign
            if (*fmt == '%')
            {
                goto put;
            }
            len = 0;
            // Parse field width
            while (*fmt >= '0' && *fmt <= '9')
            {
                len *= 10;
                len += *fmt - '0';
                fmt++;
            }
            // Skip 'l' modifier (long)
            if (*fmt == 'l')
            {
                fmt++;
            }
            // Format specifier for character
            if (*fmt == 'c')
            {
                arg = __builtin_va_arg(args, int);
                *dst++ = (char)arg;
                fmt++;
                continue;
            }
            else
                // Format specifier for decimal integer
                if (*fmt == 'd')
                {
                    arg = __builtin_va_arg(args, int);
                    // Check for negative number
                    sign = 0;
                    if ((int)arg < 0)
                    {
                        arg *= -1;
                        sign++;
                    }
                    // Cap the value to prevent overflow
                    if (arg > 99999999999999999L)
                    {
                        arg = 99999999999999999L;
                    }
                    // Convert integer to string
                    i = 18;
                    tmpstr[i] = 0;
                    do
                    {
                        tmpstr[--i] = '0' + (arg % 10);
                        arg /= 10;
                    } while (arg != 0 && i > 0);
                    if (sign)
                    {
                        tmpstr[--i] = '-';
                    }
                    // Handle field width padding
                    if (len > 0 && len < 18)
                    {
                        while (i > 18 - len)
                        {
                            tmpstr[--i] = ' ';
                        }
                    }
                    p = &tmpstr[i];
                    goto copystring;
                }
                else
                    // Format specifier for hexadecimal integer
                    if (*fmt == 'x')
                    {
                        arg = __builtin_va_arg(args, long int);
                        i = 16;
                        tmpstr[i] = 0;
                        do
                        {
                            char n = arg & 0xf;
                            tmpstr[--i] = n + (n > 9 ? 0x37 : 0x30);
                            arg >>= 4;
                        } while (arg != 0 && i > 0);
                        // Handle field width padding
                        if (len > 0 && len <= 16)
                        {
                            while (i > 16 - len)
                            {
                                tmpstr[--i] = '0';
                            }
                        }
                        p = &tmpstr[i];
                        goto copystring;
                    }
                    else
                        // Format specifier for string
                        if (*fmt == 's')
                        {
                            p = __builtin_va_arg(args, char *);
                        copystring: // Handle null pointer
                            if (p == (void *)0)
                            {
                                p = "(null)";
                            }
                            // Copy string to destination
                            while (*p)
                            {
                                *dst++ = *p++;
                            }
                        }
        }
        else
        {
            // Copy literal character to destination
        put:
            *dst++ = *fmt;
        }
        fmt++;
    }
    // Null-terminate the destination string
    *dst = 0;
    return dst - orig; // Return the number of characters written
}

unsigned int sprintf(char *dst, char *fmt, ...)
{
    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    unsigned int r = vsprintf(dst, fmt, args); // Call vsprintf with the argument list
    __builtin_va_end(args);                    // Clean up the argument list
    return r;                                  // Return the result of vsprintf
}

// Function to calculate the length of a string
unsigned long long strlen(const char *str)
{
    size_t count = 0;
    while ((unsigned char)*str++)
        count++; // Increment count for each character until the null terminator is reached
    return count;
}

// Function to compare two strings
int strcmp(const char *p1, const char *p2)
{
    const unsigned char *s1 = (const unsigned char *)p1;
    const unsigned char *s2 = (const unsigned char *)p2;
    unsigned char c1, c2;

    do
    {
        c1 = (unsigned char)*s1++; // Get the next character from each string
        c2 = (unsigned char)*s2++;
        if (c1 == '\0')
            return c1 - c2; // If the end of the first string is reached, return the difference
    } while (c1 == c2);     // Continue until characters differ or end of a string is reached
    return c1 - c2;
}

// Function to compare two strings up to n characters
int strncmp(const char *s1, const char *s2, unsigned long long n)
{
    unsigned char c1 = '\0';
    unsigned char c2 = '\0';
    if (n >= 4)
    {
        size_t n4 = n >> 2; // Divide n by 4 to get the number of 4-character blocks
        do
        {
            c1 = (unsigned char)*s1++; // Compare characters in blocks of 4
            c2 = (unsigned char)*s2++;
            if (c1 == '\0' || c1 != c2)
                return c1 - c2;
            c1 = (unsigned char)*s1++;
            c2 = (unsigned char)*s2++;
            if (c1 == '\0' || c1 != c2)
                return c1 - c2;
            c1 = (unsigned char)*s1++;
            c2 = (unsigned char)*s2++;
            if (c1 == '\0' || c1 != c2)
                return c1 - c2;
            c1 = (unsigned char)*s1++;
            c2 = (unsigned char)*s2++;
            if (c1 == '\0' || c1 != c2)
                return c1 - c2;
        } while (--n4 > 0); // Continue for all 4-character blocks
        n &= 3;             // Get the remaining number of characters to compare
    }
    while (n > 0) // Compare the remaining characters
    {
        c1 = (unsigned char)*s1++;
        c2 = (unsigned char)*s2++;
        if (c1 == '\0' || c1 != c2)
            return c1 - c2;
        n--;
    }
    return c1 - c2;
}

// Function to copy memory from source to destination
char *memcpy(void *dest, const void *src, unsigned long long len)
{
    char *d = dest;
    const char *s = src;
    while (len--)
        *d++ = *s++; // Copy each byte from source to destination
    return dest;
}
