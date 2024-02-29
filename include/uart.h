#ifndef _UART_H
#define _UART_H

void uart_init();
void uart_write(unsigned int c);
char uart_read();
void uart_printf(char *fmt, ...);
void uart_flush();

#endif
