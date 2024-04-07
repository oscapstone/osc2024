#ifndef _UART_H
#define _UART_H

#include "int.h"

void uart_init();
void uart_write(unsigned int c);

/* Receive a character */
char uart_read();

void uart_printf(char *fmt, ...);
void uart_print(char *s);
void uart_println(char *s);

/* Display a binary value in hexadecimal */
void uart_hex(size_t d);

#endif  // _UART_H
