#pragma once
#include "util.hpp"

void mini_uart_setup();
char mini_uart_getc();
void mini_uart_putc(char c);
void mini_uart_puts(const char* s);
int mini_uart_getline_echo(char* buffer, int size);
int PRINTF_FORMAT(1, 2) mini_uart_printf(const char* format, ...);
