#include "string.h"
#include "mini_uart.h"


int32_t
str_cmp(const byteptr_t s1, const byteptr_t s2)
{
    byteptr_t p1 = (byteptr_t) s1;
    byteptr_t p2 = (byteptr_t) s2;
    while (*p1 && *p1 == *p2) ++p1, ++p2;
    return (*p1 > *p2) - (*p2  > *p1);
}


uint32_t
str_eql(const byteptr_t s1, const byteptr_t s2)
{
    return str_cmp(s1, s2) == 0;
}


uint32_t
str_len(const byteptr_t str)
{
    uint32_t len = 0;
    byteptr_t p = (byteptr_t) str;
    while (*p++ != '\0') ++len;
    return len;
}


uint32_t
ascii_dec_to_uint32(const byteptr_t str, uint32_t len) {
    byteptr_t s = (byteptr_t) str;
    uint32_t num = 0;
    for (uint32_t i = 0; i < len; ++i) {
        num = num * 10;
        num += (*s - '0');
        s++;
    }
    return num;
}