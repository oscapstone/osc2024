#ifndef MINI_UART_H
#define MINI_UART_H

#include "type.h"

void mini_uart_init(void);
uint8_t mini_uart_getc(void);
uint8_t mini_uart_getb(void);
void mini_uart_putc(byte_t c);
void mini_uart_puts(byteptr_t s);
void mini_uart_putln(byteptr_t s);
void mini_uart_hex(uint32_t d);

#endif