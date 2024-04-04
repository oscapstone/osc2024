#ifndef _UART1_H
#define _UART1_H

#define MAX_BUF_SIZE 1024

void uart_init();
void uart_send_string(const char *str);
void uart_puts(const char *str);
void uart_write(unsigned int c);
char uart_read();
void uart_flush();
void uart_int(unsigned long long d);
void uart_hex(unsigned int d);
void uart_hex_64(unsigned long long d);
void disable_uart_interrupt();
void enable_uart_interrupt();
void uart_write_async(unsigned int c);
char uart_read_async();
unsigned int uart_send_string_async(const char *str);
unsigned int uart_read_string_async(char *str);
void uart_interrupt_handler();
#endif
