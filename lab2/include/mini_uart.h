#ifndef	_MINI_UART_H
#define	_MINI_UART_H

#define debug() uart_printf("%s,%s:%d\n", __FILE__, __FUNCTION__, __LINE__)

void uart_init ( void );
char uart_recv ( void );
void uart_send ( char c );
void uart_send_string(char* str);
void uart_send_hex(unsigned int *n);
void uart_send_hex64(unsigned long long *n);
void uart_printf(char* fmt, ...);

#endif  /*_MINI_UART_H */