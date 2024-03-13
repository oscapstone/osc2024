#ifndef UART_H
#define UART_H

void mini_uart_init();
char mini_uart_getc();
void mini_uart_putc( char c);
void mini_uart_puts( char *s);

#endif