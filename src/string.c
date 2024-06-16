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

/* If a == b, return 0. Else return non-zero value. */
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

/* if s1 == s2, return 0. Else return non-zero value */
int memcmp(void *s1, void *s2, int n)
{
    unsigned char *a=s1,*b=s2;
    while(n-->0){ if(*a!=*b) { return *a-*b; } a++; b++; }
    return 0;
}

int strlen(const char *str)
{
    int count = 0;
    while (*str != '\0') {
        count++;
        str++;
    }
    return count;
}

int strcpy(char *dst, const char *src)
{
    int i = 0;
    while (src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
    return i;
}

int strcat(char *dst, const char *src)
{
    int i = 0, j = 0;
    while (dst[i] != '\0')
        i++;
    while (src[j] != '\0') {
        dst[i] = src[j];
        i++;
        j++;
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

void *memset(void *str, int c, unsigned long n) {
    char *ptr = (char *)str;

    while (n-- > 0)
        *ptr++ = (char)c;
    return str;
}

void *memcpy(void *dest, const void *src, unsigned long n)
{
    char *d = dest;
    const char *s = src;

    while (n--)
        *d++ = *s++;
    return dest;
}