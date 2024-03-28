#include "headers/utils.h"

int strcmp(const char* a, const char* b)
{
    int len = strlen(b);
    if(strlen(a) != len) return 0;
    for(int i=0 ; i<len ; i++)
        if(a[i] != b[i]) return 0;
    return 1;
}

int strlen(const char *string)
{
    int len = 0;
    while(string[len] != '\0' && len<=MAX_LEN) len++;
    return len>MAX_LEN?-1:len;
}

void bin2hex(unsigned int bin, char *hex)
{
    for(int i=0 ; i<8 ; i++)
    {
        unsigned int temp = (bin>>(i<<2)) & 0xf;
        hex[7-i] = temp>9? 'a'+(temp-10) : '0'+temp;
    }
    hex[8] = '\0';
}

void bin2dec(unsigned int bin, char *dec)
{
    int dig = 0;
    
    int t = bin;
    while(t)
    {
        t /= 10;
        dig++;
    }
    
    for(int i=0 ; i<t ; i++)
    {
        dec[t-1-i] = (bin%10)+'0';
        bin /= 10;
    }
    dec[t] = '\0';
}

unsigned int atoi_dec(char *string, unsigned int size)
{
    unsigned int number = 0;
    for(unsigned int i=0 ; i<size ; i++)
    {
        number = number * 10;
        if(*string >= '0' && *string <= '9')
            number += (*string) - '0';
        else if (*string >= 'a' && *string <= 'f')
            number += (*string) - 'a' + 10;
        else if (*string >= 'A' && *string <= 'F')
            number += (*string) - 'A' + 10;
        string++;
    }
    return number;
}

unsigned int atoi(char *string, unsigned int size)
{
    unsigned int number = 0;
    for(unsigned int i=0 ; i<size ; i++)
    {
        number = number << 4;

        if(*string >= '0' && *string <= '9')
            number += (*string) - '0';
        else if (*string >= 'a' && *string <= 'f')
            number += (*string) - 'a' + 10;
        else if (*string >= 'A' && *string <= 'F')
            number += (*string) - 'A' + 10;
        string++;
    }
    return number;
}

void align(void *size, unsigned int align_size)
{
    unsigned long *a = (unsigned long*) size;
    unsigned long mask = align_size - 1;
    (*a) = ((*a) + mask) & (~mask);
}