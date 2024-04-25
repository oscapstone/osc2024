#define NULL ((void *)0)

int isalpha(char c){
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

int isdigit(int c) {
    return c >= '0' && c <= '9';
}

int toupper(int c) {
    if (c >= 'a' && c <= 'z') {
        return c - 'a' + 'A';
    } else {
        return c;
    }
}

int isxdigit(int c) {
    return isdigit(c) || (toupper(c) >= 'A' && toupper(c) <= 'F');
}

unsigned long long strlen(const char *str) {
   
    unsigned long long len = 0;
    while((unsigned char)*str++) {
        ++len;
    }
       
    return len;
}


char *strcpy(char *dest, const char *src) {
    char *ret = dest;
    while ((*dest++ = *src++));
    return ret;
}

void _strcpy(char *dest, char *src){
  if (dest==NULL||src==NULL) 
    return;
    
  while(*src!=NULL){
    *dest = *src;
    dest++;
    src++;
  }
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
    if (str != 0) {
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

long long atoi(char* str) {
    long long result = 0;
    long long sign = 1;
    long long i = 0;

    if (str[0] == '-') {
        sign = -1;
        i++;
    }

    for (; str[i] != '\0'; ++i) {
        if (str[i] >= '0' && str[i] <= '9') {
            result = result * 10 + (str[i] - '0');
        }
        else {
        
            return 0;
        }
    }

    return sign * result;
}

unsigned int parse_hex_str(char *arr, int size) {
    unsigned int result = 0;

    for (int i = 0; i < size; i++) {
        char c = arr[i];
        if (isxdigit(c)) {
            int val = isdigit(c) ? c - '0' : toupper(c) - 'A' + 10;
            result = (result << 4) + val;
        }
    }

    return result;
}