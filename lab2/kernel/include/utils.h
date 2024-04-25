#ifndef _UTILS_H_
#define _UTILS_H_

#define VSPRINT_MAX_BUF_SIZE 128

unsigned int sprintf(char *dst, char* fmt, ...);
unsigned int vsprintf(char *dst,char* fmt, __builtin_va_list args);

unsigned long long strlen(const char *str);
int                strcmp(const char*, const char*);
int                strncmp(const char*, const char*, unsigned long long);
char*              memcpy(void *dest, const void *src, unsigned long long len);
void               str_split(char* str, char delim, char** result, int* count);
char*              strcpy(char *dest, const char *src);

#endif /* _UTILS_H_ */
