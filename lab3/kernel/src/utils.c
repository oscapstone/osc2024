#include "utils.h"

uint32_t fdt_u32_le2be(const void *addr)
{
    const uint8_t *bytes = (const uint8_t *)addr;
    uint32_t ret = (uint32_t)bytes[0] << 24 | (uint32_t)bytes[1] << 16 | (uint32_t)bytes[2] << 8 | (uint32_t)bytes[3];
    return ret;
}

int my_strcmp(const char *s1, const char *s2)
{
    if (*s1 == '\0' && *s2 != '\0')
        return (*s1 - *s2);

    if (*s2 == '\0' && *s1 != '\0')
        return (*s1 - *s2);

    for (; *s1 != '\0' || *s2 != '\0'; s1++, s2++)
    {
        if ((*s1 - *s2) > 0)
            return (*s1 - *s2);
        else if ((*s1 - *s2) < 0)
            return (*s1 - *s2);
    }

    return 0;
}

int my_strncmp(const char *s1, const char *s2, int n)
{
    if (*s1 == '\0' && *s2 != '\0')
        return (*s1 - *s2);

    if (*s2 == '\0' && *s1 != '\0')
        return (*s1 - *s2);

    for (int count = 0; (*s1 != '\0' || *s2 != '\0') && count < n; s1++, s2++, count++)
    {
        if ((*s1 - *s2) > 0)
            return (*s1 - *s2);
        else if ((*s1 - *s2) < 0)
            return (*s1 - *s2);
    }

    return 0;
}

int my_strlen(const char* s) 
{
	const char* p = s;
	while (*p != '\0') 
		p++;
	return p - s;
}

int atoi(char *s)
{
    int sum = 0;
    for (int i = 0; s[i] != '\0'; i++)
        sum = sum * 10 + s[i] - '0';
    return sum;
};

int given_size_hex_atoi(char *s, int size)
{
    int sum = 0;
    for (int i = 0; i < size; i++)
    {
        if (s[i] >= 'a')
            sum = sum * 16 + 10 + (s[i] - 'a');
        else if(s[i] >= 'A')
            sum = sum * 16 + 10 + (s[i] - 'A');
        else
            sum = sum * 16 + (s[i] - '0');
    }
    return sum;
};