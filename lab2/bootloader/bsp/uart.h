#ifndef UART_H
#define UART_H

void uart_init();
char uart_recv();
void uart_puts(char *s);
void uart_hex(unsigned int d);

#endif
