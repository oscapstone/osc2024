#ifndef _UART1_H
#define _UART1_H

#include "types.h"

#define MAX_BUF_SIZE 1024

void uart_init();
void uart_send_string(const char *str);
void uart_puts(const char *str);
void uart_write(char c);
char uart_read();
void uart_flush();
void uart_int(uint64_t d);
void uart_hex(uint32_t d);
void uart_hex_64(uint64_t d);
void disable_uart_interrupt();
void enable_uart_interrupt();
void uart_write_async(char c);
char uart_read_async();
uint32_t uart_send_string_async(const char *str);
uint32_t uart_read_string_async(char *str);
void uart_interrupt_handler();
#endif
