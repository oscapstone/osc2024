#include "m_string.h"

char* m_strtok(char *str, char delim, char **next){
    if(*str == '\0')
        return str;

    // find first non delim
    while(*str == delim) str++;
    char *ret = str;
    
    // find delim or '\0'
    while(*str != '\0' && *str != delim)
        str++;

    if(*str != '\0'){
        *str = '\0';
        *next = str+1;
    }
    else {
        *next = str;
    }
    
    return ret;
}


int m_strcmp(const char * cs, const char * ct) {
	register signed char __res;
	while(1) {
		__res = *cs - *ct++;
		if(__res != 0 || !*cs++)
			break;
	}
	return __res;
}

int m_atoi(const char *s){
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

int m_htoi(const char *s){
    int num = 0;
    for(int i=0; s[i] != '\0'; ++i){
        if('0' <= s[i] && s[i] <= '9')
            num = num << 4 | (s[i] - '0');
        else if('a' <= s[i] && s[i] <= 'f')
            num = num << 4 | (s[i] - 'a' + 10);
        else if('A' <= s[i] && s[i] <= 'F')
            num = num << 4 | (s[i] - 'A' + 10);
    }
    return num;
}
char* m_itoa(int value, char *s){
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

unsigned int m_vsprintf(char *dst, char *fmt, __builtin_va_list args) {
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
                char *p = m_itoa(arg, buf);
                while (*p) {
                    *dst++ = *p++;
                }
            }
            else if (*fmt == 'c'){
                int arg = __builtin_va_arg(args, int);
                *dst++ = (char)arg;
            }
        } else {
        put:
            *dst++ = *fmt;
        }
        fmt++;
    }
    *dst = '\0';
    
    return dst - dst_orig;  // return written bytes
}

unsigned int m_sprintf(char *dst, char *fmt, ...) {
    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    return m_vsprintf(dst, fmt, args);
}
