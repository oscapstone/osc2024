#pragma once

#include "hardware.h"

#define UART_BUF_SIZE 1024

#define BACKSPACE '\b'
#define DELETE 127
#define ESC 27
#define NEWLINE '\n'
#define TAB '\t'
#define CARRIAGE_RETURN '\r'

void init_uart();
char uart_getc();
void uart_putc(char c);
void uart_hex(unsigned int h);
void uart_simple_hex(unsigned int h);
void uart_dec(unsigned int h);
void uart_puts(const char *s);

void enable_uart_tx_interrupt();
void disable_uart_tx_interrupt();
void enable_uart_rx_interrupt();
void disable_uart_rx_interrupt();

void uart_tx_irq_handler();
void uart_rx_irq_handler();

void uart_async_read(char *buf, int len);
void uart_async_write(const char *s);