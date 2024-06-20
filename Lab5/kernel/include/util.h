#ifndef _UTIL_H
#define _UTIL_H

#include "type.h"
#include "uart.h"
#include "io.h"

int strcmp(char* str1, char* str2);
int strncmp(char* str1, char* str2, uint32_t len);
void strncpy(char* src, char* dst, uint32_t len);
void memcpy(char* src, char* dst, uint32_t len);
uint32_t strlen(char* str);
int atoi(char* val_str, int len);

uint32_t to_little_endian(uint32_t val);
void delay(int);
void print_dec(int);

uint32_t read_DAIF();
uint32_t read_core_timer_enable();
uint32_t read_core_timer_expire();

#endif