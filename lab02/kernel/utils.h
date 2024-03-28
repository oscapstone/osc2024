#ifndef UTILS_H
#define UTILS_H

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
#define BUFSIZE 1024

void print_char(char);
void print_string(const char *str);
void print_h(unsigned int num);
void print_num(int num);
char read_c();
int atoi(const char *s, unsigned int size);
unsigned int strlen(const char *s);

int strncmp(const char *str1, const char *str2, uint32_t len);
char* strncpy(char *dst, const char *src, uint32_t len);
void* memset(char *dst, char c, uint32_t len);
int strcmp(char *a, char *b);
int readcmd(char *buf, int len);
#endif
