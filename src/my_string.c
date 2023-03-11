



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