#include "utils.h"

int strcmp(const char *a, const char *b)
{
    while (*a != '\0' && *b != '\0')
    {
        if (*a != *b)
            return 0;
        a++;
        b++;
    }

    return (*a == '\0' && *b == '\0');
}

void itoa(int x, char str[], int d)
{
    int i = 0;
    while (x)
    {
        str[i++] = (x % 10) + '0';
        x = x / 10;
    }

    // If number of digits required is more, then
    // add 0s at the beginning
    while (i < d)
        str[i++] = '0';

    str[i] = '\0';
    reverse(str);
}

void reverse(char *s)
{
    int i;
    char temp;

    for (i = 0; i < strlen(s) / 2; i++)
    {
        temp = s[strlen(s) - i - 1];
        s[strlen(s) - i - 1] = s[0];
        s[0] = temp;
    }
}

int strlen(char *s)
{
    int i = 0;
    while (1)
    {
        if (*(s + i) == '\0')
            break;
        i++;
    }

    return i;
}
