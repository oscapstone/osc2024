#pragma once
#include "mmio.h"
#include "util.h"

void mini_uart_setup();
char mini_uart_getc();
void mini_uart_putc(char c);
void mini_uart_puts(const char* s);
int mini_uart_getline_echo(char* buffer, int size);
