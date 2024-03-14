#ifndef MINI_UART_H
#define MINI_UART_H

void mini_uart_init(void);
char mini_uart_getc(void);
void mini_uart_putc(char c);
void mini_uart_puts(char* s);
void mini_uart_putln(char* s);
void mini_uart_hex(unsigned int d);

#endif