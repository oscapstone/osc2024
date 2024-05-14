#include <lib/string.h>
#include <lib/stdlib.h>

int strcmp(char *a, char *b) {
    while (*a && (*a == *b)) {
        a++, b++;
    }
    return *(const unsigned char *)a - *(const unsigned char *)b;
}

char *strcpy(char *dest, const char *src) {
    char *start = dest;

    while (*src != '\0') {
        *dest = *src;
        dest++;
        src++;
    }

    *dest = '\0';
    return start;
}

char *strncpy(char *dest, const char *src, int n) {
    char *start = dest;
    while (*src != '\0' && n > 0) {
        *dest = *src;
        dest++;
        src++;
        n--;
    }
    *dest = '\0';
    return start;
}

char *memcpy(char *dest, const char *src, int n) {
    char *start = dest;
    while (n > 0) {
        *dest = *src;
        dest++;
        src++;
        n--;
    }
    *dest = '\0';
    return start;
}

int strlen(const char *str) {
    int len = 0;
    while (*str) {
        str++;
        len++;
    }
    return len;
}

char *strtok(char *s, const char *delim)
{
	char *spanp;
	int c, sc;
	char *tok;
	static char *last;


	if (s == NULL && (s = last) == NULL)
	{
		return NULL;
	}

	c = *s++;
	for (spanp = (char*)delim; (sc = *spanp++); ) {
		if (c == sc) {
			c = *s++;
			spanp = (char*)delim;
		}
	}

	if (c == 0) {		
		last = NULL;
		return (NULL);
	}
	tok = s - 1;


	while(1) {
		c = *s++;
		spanp = (char *)delim;
		do {
			if ((sc = *spanp++) == c) 
			{
				if (c == 0)
				{
					s = NULL;
				}
				else
				{
					s[-1] = 0;
				}
				last = s;
				return (tok);
			}
		} while (sc != 0);
	}
}
