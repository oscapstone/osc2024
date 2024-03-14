#include "string.h"
#include "math.h"

// Compare two strings s1 and s2
int strcmp(char* s1, char* s2) {
    int i;

    // Iterate over each character in s1
    for (i = 0; i < strlen(s1); i++) {
        // If characters at the current position are different, return the difference
        if (s1[i] != s2[i]) {
            return s1[i] - s2[i];
        }
    }

    // If all characters are the same, return the difference of the null terminators
    return s1[i] - s2[i];
}

// Set the first 'size' characters of string s1 to character 'c'
void strset(char* s1, int c, int size) {
    int i;

    // Iterate over the first 'size' characters of s1
    for (i = 0; i < size; i++) s1[i] = c;
}

// Calculate the length of a string s
int strlen(char* s) {
    int i = 0;
    
    // Iterate over each character in s until the null terminator is found
    while (1) {
        if (*(s + i) == '\0')
            break;
        i++;
    }

     // Return the length of the string (not including the null terminator)
    return i;
}