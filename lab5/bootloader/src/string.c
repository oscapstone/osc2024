#include "stddef.h"

size_t strlen(const char *str)
{
	size_t count = 0;
	while ((unsigned char)*str++)
		count++;
	return count;
}

int strcmp(const char *p1, const char *p2)
{
	const unsigned char *s1 = (const unsigned char *)p1;
	const unsigned char *s2 = (const unsigned char *)p2;
	unsigned char c1, c2;

	do
	{
		c1 = (unsigned char)*s1++;
		c2 = (unsigned char)*s2++;
		if (c1 == '\0')
			return c1 - c2;
	} while (c1 == c2);
	return c1 - c2;
}

int strncmp(const char *s1, const char *s2, unsigned long long n)
{
	unsigned char c1 = '\0';
	unsigned char c2 = '\0';
	if (n >= 4)
	{
		size_t n4 = n >> 2;
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

char *strcpy(char *dest, const char *src)
{
	while ((*dest++ = *src++))
		;
	return dest;
}
