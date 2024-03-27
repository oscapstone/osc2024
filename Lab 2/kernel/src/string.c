#include "string.h"


static uint32_t 
_is_delim(uint8_t c)
{
    return (c == ' ' || c == '\t' || c == '\n') ? 1 : 0;
}

byteptr_t
str_tok(byteptr_t s)
{
    static byteptr_t old_s;
    if (!s) s = old_s;
    if (!s) return nullptr;
    // find begin
    while (_is_delim(*s)) { s++; }
    // reached the end
    if (*s == '\0') return nullptr;
    byteptr_t ret = s;
    // find end
    while (1) {
        if (*s == '\0') {
            old_s = s;
            return ret;
        }
        if (_is_delim(*s)) {
            *s = '\0';
            old_s = s + 1;
            return ret;
        }
        s++;
    }
    return nullptr;
}


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


static void 
_reverse(byteptr_t str, uint32_t length)
{
    uint32_t start = 0;
    uint32_t end = length - 1;
    while (start < end) {
        uint8_t temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        end--;
        start++;
    }
}


void 
uint32_to_hex(uint32_t n, byteptr_t buffer) 
{
    uint32_t length = 0;
    while (n != 0) {
        int rem = n % 16;
        buffer[length++] = (rem < 10) ? rem + '0' : rem + 'A';
        n = n / 16;
    }
    if (length == 0) buffer[length++] = '0';
    buffer[length] = '\0';
    _reverse(buffer, length);
}


void 
uint32_to_ascii(uint32_t n, byteptr_t buffer)
{
    uint32_t length = 0;   
    while (n != 0) {
        int rem = n % 10;
        buffer[length++] = rem + '0';
        n = n / 10;
    }
    if (length == 0) buffer[length++] = '0';
    buffer[length] = '\0';
    _reverse(buffer, length);
}


uint32_t
ascii_to_uint32(const byteptr_t str, uint32_t len) {
    byteptr_t s = (byteptr_t) str;
    uint32_t num = 0;
    for (uint32_t i = 0; i < len; ++i) {
        num = num << 4;
        if (*s >= 'A' && *s <= 'F') {
            num += (*s - 'A' + 10);
        } else if (*s >= 'a' && *s <= 'f') {
            num += (*s - 'a' + 10);
        } else {
            num += (*s - '0');
        }
        s++;
    }
    return num;
}


uint64_t
ascii_to_uint64(const byteptr_t str, uint32_t len) {
    byteptr_t s = (byteptr_t) str;
    uint64_t num = 0;
    for (uint32_t i = 0; i < len; ++i) {
        num = num << 4;
        if (*s >= 'A' && *s <= 'F') {
            num += (*s - 'A' + 10);
        } else if (*s >= 'a' && *s <= 'f') {
            num += (*s - 'a' + 10);
        } else {
            num += (*s - '0');
        }
        s++;
    }
    return num;
}


uint32_t
ascii_dec_to_uint32(const byteptr_t str) {
    byteptr_t s = (byteptr_t) str;
    uint32_t value = 0;
    while (*s) {
        value = value * 10 + (*s - '0');
        ++s;
    }
    return value;
}