#pragma once
#include "mmio.h"
#include "util.h"

void mini_uart_setup();
char mini_uart_getc();
void mini_uart_putc(char c);
