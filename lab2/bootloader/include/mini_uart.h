#ifndef _MINI_UART_H
#define _MINI_UART_H

void uart_init(void);
char uart_recv_raw(void);
char uart_recv(void);

void uart_send_raw(char c);
void uart_send(char c);
void uart_send_byte(char* byte);
void uart_send_string(char* str);
void uart_send_hex(unsigned int* n);
void uart_send_hex64(unsigned long long* n);

void uart_printf(char* fmt, ...);

#endif /*_MINI_UART_H */