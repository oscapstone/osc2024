#ifndef _UART_H
#define _UART_H

void uart_init();
void uart_send_string(char *str);
void uart_write(unsigned int c);
char uart_read();
void uart_printf(char *fmt, ...);
void uart_flush();
void uart_hex(unsigned int d);

#endif
