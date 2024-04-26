#ifndef UART_H
#define UART_H

#include "base.h"

#define TXD 14
#define RXD 15

#define UART_BUFFER_SIZE 256

void uart_init();
void uart_send_char(unsigned int c);
char uart_get_char();
void uart_send_string(const char* s);
void uart_send_nstring(unsigned int length, const char* s);
void uart_binary_to_hex(unsigned int d);
void uart_hex64(U64 value);

void uart_enable_int();
void uart_disable_int();

// only for interrupt
void uart_handle_int();
void uart_set_transmit_int();

BOOL uart_async_empty();
char uart_async_get_char();
int uart_async_write_char(U8 c);

#endif