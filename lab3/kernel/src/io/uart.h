#ifndef UART_H
#define UART_H

#include "base.h"

void uart_init();
void uart_send_char(unsigned int c);
char uart_get_char();
void uart_send_string(const char* s);
void uart_send_nstring(unsigned int length, const char* s);
void uart_binary_to_hex(unsigned int d);
void uart_hex64(U64 value);

// only for interrupt
void uart_handle_int();

BOOL uart_async_empty();
U8 uart_async_get_char();

#endif