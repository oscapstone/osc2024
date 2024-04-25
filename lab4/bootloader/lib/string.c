
int isalpha(char c){
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

int strlen(const char *s) {
    int len = 0;
    while (*s++) len++;
    return len;
}

char *strcpy(char *dest, const char *src) {
    char *ret = dest;
    while ((*dest++ = *src++));
    return ret;
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && *s2 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *s1 - *s2;
}

int strncmp(const char *s1, const char *s2, int n) {
    while (n-- && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    if (n == (int) -1) {
        return 0;
    }
    return *(const unsigned char *) s1 - *(const unsigned char *) s2;
}

char* strtok(char* str, const char* delimiters) {
    static char* buffer = 0;
    if (str != 0){
        buffer = str;
    }
    if (buffer == 0) {
        return 0;
    }
    char* start = buffer;
    while (*buffer != '\0') {
        const char* delim = delimiters;
        while (*delim != '\0') {
            if (*buffer == *delim) {
                *buffer = '\0';
                buffer++;
                if (start != buffer) {
                    return start;
                } else {
                    start++;
                    break;
                }
            }
            delim++;
        }
        if (*delim == '\0') {
            buffer++;
        }
    }
    if (start == buffer) {
        return 0;
    } else {
        return start;
    }
}

int atoi(char *str)
{
    int res = 0;

    for (int i = 0; str[i] != '\0'; ++i)
    {
        if (str[i] > '9' || str[i] < '0')
            return res;
        res = res * 10 + str[i] - '0';
    }

    return res;
}