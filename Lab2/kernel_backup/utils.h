#ifndef _UTILS_H
#define _UTILS_H

int oct2bin(char *s, int n);
int hex2bin(char *s, int n);
void uart_int(int i);
int atoi(char * c);
void* simple_malloc(void **now, int size);

#endif