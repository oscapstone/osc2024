#include <lib/stdlib.h>

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

void itoa(int value, char *str) {
    int i = 0;
    int sign = value;
    if (value < 0) value = -value;
    do {
        str[i++] = value % 10 + '0';
    } while ((value /= 10) > 0);
    if (sign < 0) str[i++] = '-';
    str[i] = '\0';

    // reverse
    for (int j = 0; j < i / 2; j++) {
        char temp = str[j];
        str[j] = str[i - j - 1];
        str[i - j - 1] = temp;
    }
}

