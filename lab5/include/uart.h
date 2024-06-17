#pragma once

#include "hardware.h"

#define UART_BUF_SIZE 1024

#define BACKSPACE '\b'
#define DELETE 127
#define ESC 27
#define NEWLINE '\n'
#define TAB '\t'
#define CARRIAGE_RETURN '\r'

#define INFO 0
#define WARN 1
#define TEST 2
#define BUDD 10
#define CACH 11
#define ERR 255

void init_uart();
char uart_getc();
void uart_putc(char c);
void uart_puts(const char *s);
void uart_clear();
void uart_hex(unsigned int h);
void uart_simple_hex(unsigned int h);
void uart_dec(unsigned int h);
void uart_log(int type, const char *msg);

void enable_uart_tx_interrupt();
void disable_uart_tx_interrupt();
void enable_uart_rx_interrupt();
void disable_uart_rx_interrupt();

void uart_tx_irq_handler();
void uart_rx_irq_handler();

void uart_async_read(char *buf, int len);
void uart_async_write(const char *s);