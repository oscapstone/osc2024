#ifndef _UTILS_H_
#define _UTILS_H_

#define VSPRINT_MAX_BUF_SIZE 128

unsigned int sprintf(char* dst, char* fmt, ...);
unsigned int vsprintf(char* dst, char* fmt, __builtin_va_list args);

int strcmp(const char*, const char*);

#endif /* _UTILS_H_ */