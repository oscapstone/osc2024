#include "string.h"

int strcmp(const char* a, const char* b){
    while(*a == *b){
        if(*a == '\0')
            return 0;
        a++;
        b++;
    }
    return *a - *b;
}