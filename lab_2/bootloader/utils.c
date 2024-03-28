#define PM_PASSWORD 0x5a000000
#define PM_RSTC 0x3F10001c
#define PM_WDOG 0x3F100024

/**
 * compare two string, given s1 and s2
 * return 0 if the both string are equal;
 * return -1 if s1 is less than s2;
 * return 1 if s1 is greater than s2
 */
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

/**
 * set up given value to given address
 */
void set(long addr, unsigned int value) {
    volatile unsigned int* point = (unsigned int*)addr;
    *point = value;
}


/**
 * reboot respberry pi
 * if tick=0, reboot instantly;
 * otherwise, wait for tick seconds to reboot
 */
void reset(int tick) {                 // reboot after watchdog timer expire
    set(PM_RSTC, PM_PASSWORD | 0x20);  // full reset
    if(tick != 0){
        set(PM_WDOG, PM_PASSWORD | tick);  // number of watchdog tick
    }
}

/**
 * clear reboot register
 */
void cancel_reset() {
    set(PM_RSTC, PM_PASSWORD | 0);  // full reset
    set(PM_WDOG, PM_PASSWORD | 0);  // number of watchdog tick
}