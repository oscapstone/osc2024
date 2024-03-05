#include "my_math.h"
#include "uart0.h"
#include "utli.h"

unsigned int vsprintf(char *dst, char *fmt, __builtin_va_list args) {
    char *dst_orig = dst;
    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            if (*fmt == '%') { // escape %
                goto put;
            }
            if (*fmt == 's') { // string
                char *p = __builtin_va_arg(args, char *);
                while (*p) {
                    *dst++ = *p++;
                }
            }
            if (*fmt == 'd') { // number
                int arg = __builtin_va_arg(args, int);
                char buf[11];
                char *p = itoa(arg, buf);
                while (*p) {
                    *dst++ = *p++;
                }
            }
            if (*fmt == 'x') { // hex
                int arg = __builtin_va_arg(args, int);
                char buf[8 + 1];
                char *p = itox(arg, buf);
                while (*p) {
                    *dst++ = *p++;
                }
            }
            if (*fmt == 'f') { // float
                float arg = (float)__builtin_va_arg(args, double);
                char buf[19]; // sign + 10 int + dot + 6 float
                char *p = ftoa(arg, buf);
                while (*p) {
                    *dst++ = *p++;
                }
            }
        } else {
        put:
            *dst++ = *fmt;
        }
        fmt++;
    }
    *dst = '\0';
    return dst - dst_orig; // return written bytes
}

unsigned int sprintf(char *dst, char *fmt, ...) {
    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    return vsprintf(dst, fmt, args);
}

int strcmp(const char *X, const char *Y) {
    while (*X) {
        if (*X != *Y)
            break;
        X++;
        Y++;
    }
    return *(const unsigned char *)X - *(const unsigned char *)Y;
}

int strncmp(const char *X, const char *Y, unsigned int n) {
    for (int i = 0; i < n; i++) {
        if (!X || !Y) {
            if (!X && !Y) {
                return 0;
            } else {
                return -1;
            }
        } else {
            if (*X != *Y) {
                return *(const unsigned char *)X - *(const unsigned char *)Y;
            }
        }
    }
    return 0;
}