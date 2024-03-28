#ifndef _UTILS_H
#define _UTILS_H

#define NULL ((void*)0)

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef unsigned long int uintptr_t;

#define BUFSIZE 1024

int atoi(const char *str, unsigned int size);
void read_cmd(char* command);
int read_buf(char* buf, int len);
int strcmp(const char* command, const char* b);
int strncmp(const char *s1, const char *s2, int n);
int memcmp(const void *cs, const void *ct, unsigned long count);


unsigned int parse_hex_str(char *arr, int size);
int isxdigit(int c);
int toupper(int c);
int isdigit(int c);
int hex2int(char *s, int n);

char* strncpy(char *dest, const char *src, uint32_t n);

uint32_t utils_align_up(uint32_t size, int alignment);
int utils_string_compare(const char* str1,const char* str2);
uint32_t utils_strlen(const char *s);

#endif