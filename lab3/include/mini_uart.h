#ifndef	_MINI_UART_H
#define	_MINI_UART_H

#define debug(mesg) uart_printf("%s,%s:%d Mesg: %s\n", __FILE__, __FUNCTION__, __LINE__, mesg)

void uart_init ( void );
char uart_recv ( void );
void uart_send ( char c );
void uart_send_string(char* str);
void uart_send_hex(unsigned int *n);
void uart_send_hex64(unsigned long long *n);
void uart_printf(char* fmt, ...);

#endif  /*_MINI_UART_H */