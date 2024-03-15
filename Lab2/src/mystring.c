#include "mymath.h"
#include "../include/peripherals/mbox.h"
#include "uart.h"
// int to hex
char *itox(unsigned int value, char *s) {
    int idx = 0;

    char tmp[8 + 1];
    int tidx = 0;
    if (value == 0){
        for (int i=0; i<9 ;i++){
            s[i] = '0';
            if (i==8){
                s[i] = '\0';
            }
        }
        return s;
    }
    else{
        while (value) {
            int r = value % 16;
            if (r < 10) {
                tmp[tidx++] = '0' + r;
            }
            else {
                tmp[tidx++] = 'a' + r - 10;
            }
            value /= 16;
        }
    }
    // reverse tmp
    int i;
    for (i = tidx - 1; i >= 0; i--) {
        s[idx++] = tmp[i];
    }
    s[idx] = '\0';

    return s;
}


// int to char
char *itoa(int value, char *s){
    int idx = 0;
    // check if negative
    if (value < 0) {
        value *= -1;
        s[idx++] = '-';
    }
    char tmp[10];
    int tidx = 0;
    do {
        tmp[tidx++] = '0' + value % 10;
        value /= 10;
    } while (value != 0 && tidx < 11);

    // reverse tmp
    int i;
    for (i = tidx - 1; i >= 0; i--) {
        s[idx++] = tmp[i];
    }
    s[idx] = '\0';

    return s;
}
char *ftoa(float value, char *s) {
    int idx = 0;
    if (value < 0) {
        value = -value;
        s[idx++] = '-';
    }

    int ipart = (int)value;
    float fpart = value - (float)ipart;

    // convert ipart
    char istr[11];  // 10 digit
    itoa(ipart, istr);

    // convert fpart
    char fstr[7];  // 6 digit
    fpart *= (int)pow(10, 6);
    itoa((int)fpart, fstr);

    // copy int part
    char *ptr = istr;
    while (*ptr) s[idx++] = *ptr++;
    s[idx++] = '.';
    // copy float part
    ptr = fstr;
    while (*ptr) s[idx++] = *ptr++;
    s[idx] = '\0';

    return s;
}

int strcmp(const char *X, const char *Y){
    while(*X){
        if (*X != *Y){
            break;
        }
        X++;
        Y++;
    }
    return *(const unsigned char *)X - *(const unsigned char *)Y;
}

int strcmpl(const char *X, const char *Y, int len){
    for (int i=0; i<len; i++){
        if (*X != *Y){
            return 1;
        }
        X++;
        Y++;
    }
    return 0;
}
