#ifndef _UART1_H
#define _UART1_H

void uart1_init();
void uart1_send_string(char *str);
void uart1_write(unsigned int c);
char uart1_read();
void uart1_printf(char *fmt, ...);
void uart1_flush();
void uart1_hex(unsigned int d);

#endif
