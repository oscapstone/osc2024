#ifndef	_MINI_UART_H
#define	_MINI_UART_H

#include <sys/types.h>
#include <stdint.h>

void uart_init ( void );
char uart_recv ( void );
void uart_send ( char c );
void uart_send_string(char* str);
void uart_recv_string( char* buf );
uint32_t uart_recv_uint(void);
void output( char*);
void output_hex( unsigned int );

void uart_printf(char* fmt, ...);
void uart_send_num(int64_t, int, int);

#endif  /*_MINI_UART_H */
