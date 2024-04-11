#ifndef _U_STRING_H_
#define _U_STRING_H_

#define VSPRINT_MAX_BUF_SIZE 0x100 // 256

unsigned int sprintf(char *dst, char* fmt, ...);
unsigned int vsprintf(char *dst,char* fmt, __builtin_va_list args);

unsigned long long strlen(const char *str);
int                strcmp(const char*, const char*);
int                strncmp(const char*, const char*, unsigned long long);
char*              memcpy(void *dest, const void *src, unsigned long long len);
char*              strcpy(char *dest, const char *src);

char* str_SepbySpace(char* head);
int   atoi(char* str);

#endif /* _U_STRING_H_ */
