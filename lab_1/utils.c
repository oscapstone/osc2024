#include "utils.h"

int strcmp(char* a, char* b){
    int value = 0;
    while(*a != '\0' && *b != '\0'){
        if(*a < *b){
            return -1;
        }
        else if(*a > *b){
            return 1;
        }
        else{
            a++;
            b++;
        }
    }
    if(*a == '\0'){
        value++;
    }
    if(*b == '\0'){
        value--;
    }
    return value;
}

void set(long addr, unsigned int value) {
    volatile unsigned int* point = (unsigned int*)addr;
    *point = value;
}

void reset() {                 // reboot after watchdog timer expire
    set(PM_RSTC, PM_PASSWORD | 0x20);  // full reset
    set(PM_WDOG, PM_PASSWORD | 10);  // number of watchdog tick
}

