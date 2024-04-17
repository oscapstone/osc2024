#ifndef __IO_H__
#define __IO_H__

#include "mini_uart.h"

void printf(char* str);
void printfc(char c);
void printf_hex(unsigned int d);
void printf_int(int d);
char read_char();

#define debug() printf("\r\n [DEBUG] : "); printf(__FILE__); printf(" : "); printf_int(__LINE__)

#endif