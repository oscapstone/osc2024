#include "utils.h"

int strlen(const char *s)
{
    int i = 0;
    while (*(s + i) != '\0')
        i++;

    return i;
}

int strcmp(const char *s1, const char *s2)
{
    int i = 0;

    for (i = 0; i < strlen(s1); i++)
    {
        if (s1[i] != s2[i])
        {
            return s1[i] - s2[i];
        }
    }

    return s1[i] - s2[i];
}

int strncmp(const char *s1, const char *s2, unsigned long long n)
{
    unsigned char c1 = '\0';
    unsigned char c2 = '\0';
    if (n >= 4)
    {
        unsigned int n4 = n >> 2;
        do
        {
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
            c1 = (unsigned char)*s1++;
            c2 = (unsigned char)*s2++;
            if (c1 == '\0' || c1 != c2)
                return c1 - c2;
        } while (--n4 > 0);
        n &= 3;
    }
    while (n > 0)
    {
        c1 = (unsigned char)*s1++;
        c2 = (unsigned char)*s2++;
        if (c1 == '\0' || c1 != c2)
            return c1 - c2;
        n--;
    }
    return c1 - c2;
}

unsigned int hex_to_dec(char *s)
{
    unsigned int ret = 0;
    while (*s != '\0')
    {
        ret *= 16;
        if (*s >= '0' && *s <= '9')
            ret += *s - '0';
        else if (*s >= 'a' && *s <= 'f')
            ret += *s - 'a' + 10;
        else if (*s >= 'A' && *s <= 'F')
            ret += *s - 'A' + 10;
        else
            return ret;
        s++;
    }
    return ret;
}

unsigned int hex_to_decn(char *s, unsigned int len)
{
    unsigned int ret = 0;
    for (unsigned int i = 0; i < len; i++)
    {
        ret *= 16;
        if (s[i] >= '0' && s[i] <= '9')
            ret += s[i] - '0';
        else if (s[i] >= 'a' && s[i] <= 'f')
            ret += s[i] - 'a' + 10;
        else if (s[i] >= 'A' && s[i] <= 'F')
            ret += s[i] - 'A' + 10;
        else
            return ret;
    }
    return ret;
}
