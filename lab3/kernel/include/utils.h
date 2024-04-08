#ifndef __UTILS_H
#define __UTILS_H

typedef unsigned int uint32_t;
typedef unsigned char uint8_t;

uint32_t fdt_u32_le2be(const void *addr);
int my_strcmp(const char *s1, const char *s2);
int my_strncmp(const char *s1, const char *s2, int n);
int my_strlen(const char* s) ;
int atoi(char *s);
int given_size_hex_atoi(char *s, int size);

#endif