#include "utils.h"

int strcmp(char *a, char *b) //return 0 if two string(char array) is equal
{
    while (*a)
    {
        if (*a != *b)
        {
            break;
        }
        a++;
        b++;
    }

    return *a - *b; //return first element difference of the char array
}

unsigned long utils_atoi(const char *s, int char_size) {
    unsigned long num = 0;
    for (int i = 0; i < char_size; i++) {
        num = num * 16;
        if (*s >= '0' && *s <= '9') {
            num += (*s - '0');

        } 
        
        else if (*s >= 'A' && *s <= 'F') {
            num += (*s - 'A' + 10);
        } 
        
        else if (*s >= 'a' && *s <= 'f') {
            num += (*s - 'a' + 10);
        }
        
        s++;
    }

    return num;
}

void utils_align(void *size, unsigned int s) {
    unsigned long* x = (unsigned long*) size;
    unsigned long mask = s-1;
    *x = ((*x) + mask) & (~mask);
}

uint32_t utils_align_up(uint32_t size, int alignment) {
  return (size + alignment - 1) & ~(alignment-1);
}

size_t utils_strlen(const char *s) {
    size_t i = 0;
    while (s[i]) 
      i++;

    return i+1;
}

