int strcmp(const char *str1, const char *str2)
{
    const char *ptr1 = str1;
    const char *ptr2 = str2;
    while(1)
    {
        char c1 = *ptr1;
        ptr1++;
        char c2 = *ptr2;
        ptr2++;

        if(c1 == c2)
        {
            if(c1 == '\0') return 0;
            else continue;
        }
        else 
        {
            return 1;
        }
    }
}

int strncmp(const char *str1, const char *str2, int len)
{
    const char *ptr1 = str1;
    const char *ptr2 = str2;
    int cnt = 0;
    while(cnt < len)
    {
        char c1 = *ptr1;
        ptr1++;
        char c2 = *ptr2;
        ptr2++;
        cnt++;

        if(c1 != c2)
            return 1;
    }

    return 0;
}

char *strncpy(char *dest, const char *src, int len) 
{
	while (len--)
		*dest++ = *src++;

	return dest;
}

int strlen(const char *str) 
{
	int len = 0;
	while (*str++ != '\0')
		len++;
	return len;
}

int memcmp(const void *str1, const void *str2, int len) 
{
    const unsigned char *ptr1 = str1;
    const unsigned char *ptr2 = str2;
	while (len--) 
    {
		if (*ptr1 != *ptr2)
			return *ptr1 - *ptr2;
            
		ptr1++;
		ptr2++;
	}

	return 0;
}

unsigned int is_visible(unsigned int c)
{
    if(c >= 32 && c <= 126){
        return 1;
    }
    return 0;
}

int hextoi(const char *s, int len)
{
	int r = 0;
	while (len--) {
		r = r << 4;
        if (*s >= 'a')
            r += *s++ - 'a' + 10;
		else if (*s >= 'A')
			r += *s++ - 'A' + 10;
		else if (*s >= '0')
			r += *s++ - '0';
	}
	return r;
}

int atoi(const char *s)
{
    int sign = 1;
    int i = 0;
    int result = 0;

    while(s[i] == ' ')
        i++;
    
    if(s[i] == '-') 
    {
        sign = -1;
        i++;
    }

    while(s[i] >= '0' && s[i] <= '9') 
    {
        result = result * 10 + (s[i] - '0');
        i++;
    }

    return sign * result;
}

unsigned int endian_big2little(unsigned int x) 
{
    return ((x & 0xFF) << 24) | ((x & 0xFF00) << 8) | ((x & 0xFF0000) >> 8) | ((x & 0xFF000000) >> 24);
}