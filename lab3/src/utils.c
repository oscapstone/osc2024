#include "utils.h"

int align4(int n)
{
    return n + (4 - n % 4) % 4;
}

int atoi(const char *s)
{
    int result = 0;
    int sign = 1;
    int i = 0;

    // Skip leading spaces
    while (s[i] == ' ') {
        i++;
    }

    // Handle positive and negative sign
    if (s[i] == '-') {
        sign = -1;
        i++;
    } else if (s[i] == '+') {
        i++;
    }

    // Convert string to integer
    while (s[i] >= '0' && s[i] <= '9') {
        result = result * 10 + (s[i] - '0');
        i++;
    }

    return sign * result;
}