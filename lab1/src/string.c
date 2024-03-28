#include "string.h"

char* __strtok_str = 0;

char* strtok(char *str, char delim){
    if(str != 0)
        __strtok_str = str;

    if(__strtok_str == 0 || *__strtok_str == '\0')
        return 0;

    // find first non delim
    while(*__strtok_str == delim)
        __strtok_str++;
    char *ret = __strtok_str;
    
    // find delim or '\0'
    while(*__strtok_str != '\0' && *__strtok_str != delim)
        __strtok_str++;

    if(*__strtok_str != '\0'){
        *__strtok_str = '\0';
        __strtok_str++;
    }
    
    return ret;
}


int strcmp(const char * cs, const char * ct) {
	register signed char __res;
	while(1) {
		__res = *cs - *ct++;
		if(__res != 0 || !*cs++)
			break;
	}
	return __res;
}

int atoi(const char *s){
    const char *p = s;
    int num = 0;
    int sign = 0;
    while(*p != '\0'){
        if(*p == '+'){
            if(sign != 0)
                break;
            sign = -1;
        }
        else if(*p == '-'){
            if(sign != 0)
                break;
            sign = 1;
        }
        else {
            int digit = *p - '0';
            if(!(digit >= 0 && digit <= 9))
                break;
            num *= 10;
            num += digit;
        }
        p++;
    }
    return (sign == 0)? num: num * sign;
}

char* itoa(int value, char *s){
    int idx = 0;
    if(value < 0) {
        value *= -1;
        s[idx++] = '-';
    }

    char tmp[10];
    int tidx = 0;
    // read from least significant digit
    do {
        tmp[tidx++] = '0' + value % 10;
        value /= 10;
    } while(value != 0 && tidx < 11);

    // reverse 
    for(int i = tidx-1; i>=0; i--)
        s[idx++] = tmp[i];

    s[idx] = '\0';
    return s;
}

// char* ftoa(float value, char *s){
//     int idx = 0;
//     if(value < 0){
//         value = -value;
//         s[idx++] = '-';
//     }

//     int ipart = (int)value;
//     float fpart = value - (float)ipart;

//     // convert ipart
//     char istr[11];
//     itoa(ipart, istr);

//     // convert fpart
//     char fstr[7];
//     fpart *= (int)pow(10, 6);
//     itoa((int)fpart, fstr);

//     char *ptr = istr;
//     while(*ptr) s[idx++] = *ptr++;
//     s[idx++] = '.';
//     ptr = fstr;
//     while(*ptr) s[idx++] = *ptr++;
//     s[idx] = '\0';

//     return s;
// }

unsigned int vsprintf(char *dst, char *fmt, __builtin_va_list args) {
    char *dst_orig = dst;

    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            // escape %
            if (*fmt == '%') {
                goto put;
            }
            // string
            else if (*fmt == 's') {
                char *p = __builtin_va_arg(args, char *);
                while (*p) {
                    *dst++ = *p++;
                }
            }
            // number
            else if (*fmt == 'd') {
                int arg = __builtin_va_arg(args, int);
                char buf[11];
                char *p = itoa(arg, buf);
                while (*p) {
                    *dst++ = *p++;
                }
            }
            else if (*fmt == 'c'){
                int arg = __builtin_va_arg(args, int);
                *dst++ = (char)arg;
            }
            // float
            // if (*fmt == 'f') {
            //     float arg = (float) __builtin_va_arg(args, double);
            //     char buf[19];  // sign + 10 int + dot + 6 float
            //     char *p = ftoa(arg, buf);
            //     while (*p) {
            //         *dst++ = *p++;
            //     }
            // }
        } else {
        put:
            *dst++ = *fmt;
        }
        fmt++;
    }
    *dst = '\0';
    
    return dst - dst_orig;  // return written bytes
}

unsigned int sprintf(char *dst, char *fmt, ...) {
    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    return vsprintf(dst, fmt, args);
}
