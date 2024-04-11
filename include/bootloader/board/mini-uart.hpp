#pragma once
#include "util.hpp"

void mini_uart_setup();
char mini_uart_getc_raw();
char mini_uart_getc();
void mini_uart_putc_raw(char c);
void mini_uart_putc(char c);
void mini_uart_puts(const char* s);
int PRINTF_FORMAT(1, 2) mini_uart_printf(const char* format, ...);
