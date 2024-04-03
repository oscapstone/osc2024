#include "string.h"

int strcmp(const char* a, const char* b)
{
    while(*a == *b){
        if(*a == '\0')
            return 0;
        a++;
        b++;
    }
    return *a - *b;
}

char* strcpy(char* dest, const char* src)
{
    char* ret = dest;
    while(*src){
        *dest = *src;
        dest++;
        src++;
    }
    *dest = '\0';
    return ret;
}

char* strncpy(char* dest, const char* src, int n)
{
    char* ret = dest;
    while(n--){
        *dest = *src;
        dest++;
        src++;
    }
    *dest = '\0';
    return ret;
}

uint32_t strlen(const char* str)
{
    uint32_t len = 0;
    while(*str){
        len++;
        str++;
    }
    return len;
}

char *strtok(char* str, const char* delim)
{
    static char* p = 0;
    if(str != 0) p = str;
    if(p == 0) return 0;
    char* ret = p;
    while(*p){
        if(*p == *delim){
            *p = '\0';
            p++;
            return ret;
        }
        p++;
    }
    return ret;
}