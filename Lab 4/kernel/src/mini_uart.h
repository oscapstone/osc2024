#ifndef __MINI_UART_H__
#define __MINI_UART_H__

#include "type.h"

void    mini_uart_init(void);

uint8_t mini_uart_getc(void);
uint8_t mini_uart_getb(void);

void    mini_uart_putc(uint8_t c);
void    mini_uart_puts(byteptr_t s);
void    mini_uart_putln(byteptr_t s);
void    mini_uart_endl();

void    mini_uart_hex(uint32_t d);
void    mini_uart_hexl(uint64_t d);

void    mini_uart_printf(char* fmt, ...);

void    mini_uart_async_demo();

void    mini_uart_tx_handler();
void    mini_uart_rx_handler();

#endif