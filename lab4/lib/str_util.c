#include "str_util.h"

int str_len(char* str) {
    int len = 0;
    while(*(str+len) != '\0') { len++; }
    return len + 1;
}

int str_cmp(const char *p1, const char *p2) {
    const unsigned char *s1 = (const unsigned char *) p1;
    const unsigned char *s2 = (const unsigned char *) p2;
    unsigned char c1, c2;
    
    c1 = (unsigned char) *s1;
    c2 = (unsigned char) *s2;
    while(c1 == c2) {
        c1 = (unsigned char) *s1++;
        c2 = (unsigned char) *s2++;
        if(c1 == '\0') { return c1 - c2; }
    }

    return c1 - c2;
}

int str_cmp_len(const char *p1, const char *p2, unsigned int len) {
    const unsigned char *s1 = (const unsigned char *) p1;
    const unsigned char *s2 = (const unsigned char *) p2;
    unsigned char c1, c2;
    
    c1 = (unsigned char) *s1;
    c2 = (unsigned char) *s2;
    for(int i = 0; i < len; i++) {
        if(c1 != c2) { break; }
        c1 = (unsigned char) *(s1 + i);
        c2 = (unsigned char) *(s2 + i);
    }

    return c1 - c2;
}

void str_reverse(char* str, int len) {
    char tmp;
    for(int i = 0, j = len - 1; i < j; i++, j--) {
        tmp = str[i];
        str[i] = str[j];
        str[j] = tmp;
    }
}

int str_to_int(char *str) {
    int val = 0;
    while(*str != '\0') {
        val = val*10 + (*str - '0');
        str++;
    }
    return val;
}

int int_to_str(int num, char* str, int base) {
    int sum = num;
    int i = 0;
    int digit;
    while(sum > 0) {
        digit = sum % base;
        if(digit <= 0xA) { str[i] = '0' + digit; }
        else { str[i] = 'A' + digit - 0xA; }
        sum /= base;
        i++;
    }
    if(sum > 0) { return -1; }
    str[i] = '\0';
    str_reverse(str, i);
    return 0;
}

int hexstr_to_int(char* str) {
    int val = 0; 
    for(int c = 0; c < 8; c++) {
        val *= 16;
        if(*str >= 'A') {
            val += 10 + (*str - 'A');
        } 
        else {
            val += (*str - '0');
        }
        str++;
    }
    return val;
} 

unsigned int swap_endian(unsigned int b_end) { // uint32_t
    unsigned int l_end; 
    unsigned int b0, b1, b2, b3;
    b0 = (b_end & 0x000000ff) << 24;
    b1 = (b_end & 0x0000ff00) << 8;
    b2 = (b_end & 0x00ff0000) >> 8;
    b3 = (b_end & 0xff000000) >> 24;
    l_end = b0 | b1 | b2 | b3;
    return l_end;
}

void *memset(void* ptr, int val, unsigned int len) {
    char* tmp = (char*)ptr;
    for(unsigned int l = 0; l < len; l++) {
        *tmp = (char) val;
        tmp++;
    }
    return ptr;
}