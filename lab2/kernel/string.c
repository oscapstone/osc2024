#include "string.h"

int strcmp(const char *s1, const char *s2)
{
	char *_s1 = (char *)s1;
	char *_s2 = (char *)s2;
	while (_s1 && _s2) {
		if (*_s1 == '\n' || *_s2 == '\0')
			return 0;
		if (*_s1 == '\r' || *_s2 == '\0')
			return 0;
		if (*_s1 != *_s2)
			return *_s1 - *_s2;
		_s1++;
		_s2++;
	}
	return *_s2 != '\0';
}
