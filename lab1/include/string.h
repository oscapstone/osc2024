#ifndef __STRING_H
#define __STRING_H


char* strtok(char *str, char delim);
int strcmp(const char * cs, const char * ct);
int atoi(const char *s);
char* itoa(int value, char *s);
// char* ftoa(float value, char *s);
unsigned int vsprintf(char *dst, char *fmt, __builtin_va_list args);
unsigned int sprintf(char *dst, char *fmt, ...);


#endif