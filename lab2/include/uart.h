#pragma once

#include "hardware.h"

void uart_init();
char uart_getc();
void uart_putc(char c);
void uart_hex(unsigned int h);
void uart_puts(const char *s);
