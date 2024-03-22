#ifndef MINI_UART_H
#define MINI_UART_H

#include "types.h"

void mini_uart_init(void);
uint8_t mini_uart_getc(void);
void mini_uart_putc(uint8_t c);
void mini_uart_puts(byteptr_t s);
void mini_uart_putln(byteptr_t s);
void mini_uart_endl();
void mini_uart_space(uint32_t n);
void mini_uart_hex(uint32_t d);
void mini_uart_hexl(uint64_t d);

#endif