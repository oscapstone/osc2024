#include "stdlib.h"

int atoi(const char *s) {
    long long result = 0;  // prevent overflow
    int sign = 1;
    int i = 0;

    while (s[i] == ' ') i++;

    if (s[i] == '-') {
        sign = -1;
        i++;
    } else if (s[i] == '+')
        i++;

    while (s[i] >= '0' && s[i] <= '9') {
        result = result * 10 + (s[i] - '0');
        i++;
    }

    return sign * (int)result;
}

void itoa(int n, char *s) {
    int i = 0;
    int sign = n;
    if (n < 0) n = -n;
    do {
        s[i++] = n % 10 + '0';
    } while ((n /= 10) > 0);
    if (sign < 0) s[i++] = '-';
    s[i] = '\0';

    // reverse
    for (int j = 0; j < i / 2; j++) {
        char temp = s[j];
        s[j] = s[i - j - 1];
        s[i - j - 1] = temp;
    }
}
