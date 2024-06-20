#include "string.h"

unsigned int str_len(const char* str)
{
    unsigned int len = 0;
    while (str[len])
        len++;
    return len;
}

int str_cmp(const char* a, const char* b)
{
    while (*a && *b && *a == *b) {
        a++;
        b++;
    }
    return *a - *b;
}

int str_n_cmp(const char* a, const char* b, unsigned int n)
{
    for (unsigned int i = 0; i < n; i++) {
        if (!a[i] || a[i] != b[i])
            return a[i] - b[i];
    }
    return 0;
}

static inline unsigned char hexchar2int(unsigned char in)
{
    const unsigned char letter = in & 0x40;
    const unsigned char shift = (letter >> 3) | (letter >> 6);
    return (in + shift) & 0xF;
}

unsigned int hexstr2int(char* hex_str)
{
    unsigned int result = 0;
    for (int i = 0; i < 8; i++)
        result = (result << 4) | hexchar2int(hex_str[i]);
    return result;
}

static char* last_str;

char* str_tok(char* str, const char* delim)
{
    if (!delim || (!str && !last_str))
        return NULL;


    /* str is NULL, continue with last_str */
    if (!str && last_str)
        str = last_str;

    /* leading delimiters */
    const char* d = delim;
    while (*str && *d) {
        for (d = delim; *d; d++) {
            if (*str == *d) {
                str++;
                break;
            }
        }
    }

    /* start of token */
    char* head = str;

    /* find end of token */
    int is_delim = 0;
    while (*str) {
        for (d = delim; *d; d++) {
            if (*str == *d) {
                is_delim = 1;
                *str = '\0';
                break;
            }
        }

        // if after found a delimiter, the next character is not a delimiter
        if (is_delim && !*d)
            break;

        str++;
    }

    last_str = *str ? str : NULL;

    return head;
}

char* str_cpy(char* dest, const char* src)
{
    char* d = dest;
    const char* s = src;
    while ((*d++ = *s++))
        ;
    return dest;
}

static inline unsigned int decchar2int(unsigned char in)
{
    return in - '0';
}

unsigned int decstr2int(char* dec_str)
{
    unsigned int result = 0;
    while (*dec_str)
        result = result * 10 + decchar2int(*dec_str++);
    return result;
}
