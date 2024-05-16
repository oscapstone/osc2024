#ifndef __IO_H__
#define __IO_H__

#include "mini_uart.h"

void printf(char* str);
void printfc(char c);
void printf_hex(unsigned int d);
char read_char();

#endif