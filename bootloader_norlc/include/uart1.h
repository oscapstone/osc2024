#ifndef _UART1_H
#define _UART1_H

void uart_init();
void uart_send_string(const char *str);
void uart_write(unsigned int c);
char uart_read();
char uart_read_raw();
void uart_flush();
void uart_int(unsigned int d);
void uart_hex(unsigned int d);
void uart_hex_64(unsigned long int d);

#endif
