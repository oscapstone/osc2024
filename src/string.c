/**
 * Helper function to convert ASCII octal number into binary
 * s string
 * n number of digits
 */
int oct2bin(char *s, int n)
{
    int r=0;
    while(n-- > 0) {
        r <<= 3;
        r += *s++ - '0';
    }
    return r;
}

int hex2bin(char *s, int n)
{
    int r = 0;
    while (n-- > 0) {
        r <<= 4;
        if (*s >= '0' && *s <= '9')
            r += *(s++) - '0';
        else if (*s >= 'a' && *s <= 'f')
            r += *s++ - 'a' + 10;
        else if (*s >= 'A' && *s <= 'F')
            r += *s++ - 'A' + 10;
        else
            return -1;
    }
    return r;
}

int strcmp(const char *a, const char *b)
{
    while (*a) {
        if (*a != *b)
            break;
        a++;
        b++;
    }
    return *(const unsigned char *)a - *(const unsigned char *)b;
}

int memcmp(void *s1, void *s2, int n)
{
    unsigned char *a=s1,*b=s2;
    while(n-->0){ if(*a!=*b) { return *a-*b; } a++; b++; }
    return 0;
}

int strlen(char *str)
{
    int count = 0;
    while (*str != '\0') {
        count++;
        str++;
    }
    return count;
}

int strcpy(char *dst, char *src)
{
    int i = 0;
    while (src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
    return i;
}

int atoi(char *s)
{
    int n = 0, sign = 1;
    if (*s == '-') {
        sign = -1;
        s++;
    }
    while (*s >= '0' && *s <= '9') {
        n = n * 10 + (*s - '0');
        s++;
    }
    return n * sign;
}