#pragma once

#include "hardware.h"

#define UART_BUF_SIZE 1024

#define BACKSPACE '\b'
#define DELETE 127
#define NEWLINE '\n'
#define TAB '\t'
#define CARRIAGE_RETURN '\r'

void uart_init();
char uart_getc();
void uart_putc(char c);
void uart_hex(unsigned int h);
void uart_puts(const char *s);

void uart_enable_tx_interrupt();
void uart_disable_tx_interrupt();
void uart_enable_rx_interrupt();
void uart_disable_rx_interrupt();

void uart_tx_irq_handler();
void uart_rx_irq_handler();

void uart_async_read(char *buf, int len);
void uart_async_write(const char *s);