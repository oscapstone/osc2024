#include "../include/my_string.h"


void strset (char * s1, int c, int size )
{
    int i;

    for ( i = 0; i < size; i ++)
        s1[i] = c;
}

int strlen (const char * s )
{
    int i = 0;
    while ( 1 )
    {
        if ( *(s+i) == '\0' )
            break;
        i++;
    }

    return i;
}

int strncmp(const char *s1, const char *s2, size_t n) {
    while (n-- > 0) {
        if (*s1 != *s2) return *(unsigned char *)s1 - *(unsigned char *)s2;
        if (*s1 == '\0') return 0;
        s1++;
        s2++;
    }
    return 0;
}

// Custom strcmp function
int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

// Custom itoa function
void itoa(int num, char *str, int base) {
    int i = 0;
    int is_negative = 0;

    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }

    if (num < 0 && base == 10) {
        is_negative = 1;
        num = -num;
    }

    while (num != 0) {
        int rem = num % base;
        str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        num = num / base;
    }

    if (is_negative) str[i++] = '-';

    str[i] = '\0';

    int start = 0;
    int end = i - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}


unsigned int hex_to_uint(char *hex) {
    unsigned int result = 0;
    for (int i = 0; i < 8; i++) {  // Assuming each field contains 8 hexadecimal characters
        result = (result << 4) | HEX_DIGIT_TO_INT(hex[i]);
    }
    return result;
}

char *strrchr(const char *str, int ch) {
    const char *last_occurrence = NULL;

    // Traverse the string from beginning to end
    while (*str != '\0') {
        if (*str == ch) {
            last_occurrence = str; // Update last occurrence pointer
        }
        str++;
    }

    // Special case: check for '\0' (null terminator) if looking for '\0'
    if (ch == '\0') {
        return (char*)str; // Return pointer to the null terminator
    }

    return (char*)last_occurrence; // Return pointer to the last occurrence of ch
}

unsigned long long strtoull(const char *str, char **endptr, int base) {
    unsigned long long result = 0;
    int negative = 0;

    // Skip leading whitespace
    while (*str == ' ' || (*str >= '\t' && *str <= '\r')) {
        str++;
    }

    // Handle optional sign
    if (*str == '+' || *str == '-') {
        negative = (*str == '-');
        str++;
    }

    // Determine base if not specified
    if (base == 0) {
        if (*str == '0') {
            str++;
            if (*str == 'x' || *str == 'X') {
                base = 16;
                str++;
            } else {
                base = 8;
            }
        } else {
            base = 10;
        }
    } else if (base == 16 && *str == '0' && (*(str + 1) == 'x' || *(str + 1) == 'X')) {
        str += 2;
    }

    // Convert characters to unsigned long long integer
    while (*str != '\0') {
        int digit;

        if (*str >= '0' && *str <= '9') {
            digit = *str - '0';
        } else if (*str >= 'a' && *str <= 'z') {
            digit = *str - 'a' + 10;
        } else if (*str >= 'A' && *str <= 'Z') {
            digit = *str - 'A' + 10;
        } else {
            break; // Invalid character encountered
        }

        if (digit >= base) {
            break; // Digit out of range for current base
        }

        result = result * base + digit;
        str++;
    }

    // Set endptr if it's not NULL
    if (endptr != NULL) {
        *endptr = (char*)str;
    }

    // Handle negative sign
    if (negative) {
        result = -result;
    }

    return result;
}

char *strtok(char *str, const char *delim) {
    static char *next_token = NULL; 
    // If str is NULL, continue with the last token found
    if (str == NULL && next_token == NULL) {
        return NULL;
    }

    // Initialize str to start tokenization
    if (str != NULL) {
        next_token = str;
    }

    // Skip leading delimiters
    while (*next_token != '\0' && strchr(delim, *next_token) != NULL) {
        next_token++;
    }

    // If reached end of string, return NULL
    if (*next_token == '\0') {
        next_token = NULL;
        return NULL;
    }

    // Save the beginning of the token
    char *token = next_token;

    // Find the end of the token
    while (*next_token != '\0' && strchr(delim, *next_token) == NULL) {
        next_token++;
    }

    // If not reached end of string, replace delimiter with '\0'
    if (*next_token != '\0') {
        *next_token = '\0';
        next_token++;
    } else {
        next_token = NULL; // Mark end of tokenization
    }

    return token;
}

char *strcpy(char *dest, const char *src) {
    char *original_dest = dest; // Store original destination pointer

    // Copy characters from src to dest
    while ((*dest++ = *src++) != '\0')
        ;

    return original_dest; // Return original destination pointer
}

char *strchr(const char *str, int c) {
    while (*str != '\0') {
        if (*str == c) {
            return (char *)str;  // Cast away constness for return type compatibility
        }
        str++;
    }

    if (c == '\0') {
        return (char *)str;  // Return pointer to null terminator if c is '\0'
    }

    return NULL;  // Character not found
}

int atoi(const char *str) {
    int result = 0;
    int sign = 1;
    int i = 0;

    // Handle optional leading whitespace
    while (str[i] == ' ') {
        i++;
    }

    // Handle optional sign
    if (str[i] == '-') {
        sign = -1;
        i++;
    } else if (str[i] == '+') {
        i++;
    }

    // Convert digits to integer
    while (str[i] >= '0' && str[i] <= '9') {
        result = result * 10 + (str[i] - '0');
        i++;
    }

    // Apply sign
    result *= sign;

    return result;
}
