#ifndef _UART_H
#define _UART_H

void uart_init();
void uart_write(unsigned int c);
char uart_read();

void uart_print(char *s);
void uart_println(char *s);

#endif // _UART_H
