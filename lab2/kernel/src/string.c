# include "string.h"

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

int strcmp(const char* str1, const char* str2) 
{
    while (*str1 != '\0' && *str2 != '\0') {
        if (*str1 != *str2) {
            return 0;
        }
        str1++;
        str2++;
    }
    if (*str1 == '\0' && *str2 =='\0') {
        return 1;
    } else {
        return 0;
    }
}

unsigned long long strlen(const char *str)
{
    int count = 0;
    while((unsigned char)*str++)count++;
    return count;
}