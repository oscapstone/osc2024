#ifndef __UART_H__
#define __UART_H__
#include "types.h"

void uart_init();
void uart_send(unsigned int c);
char uart_getc();
void uart_puts(char *s);
void uart_hex(unsigned int d);

void delay(int time);
void enable_read_interrupt(void);
void disable_read_interrupt(void);
void enable_write_interrupt(void);
void disable_write_interrupt(void);
void enable_uart_interrupt(void);
void disable_uart_interrupt(void);
void async_uart_handler(void);
void async_uart_puts(const char *s);
uint32_t async_uart_gets(char *buffer, uint32_t size);

#endif