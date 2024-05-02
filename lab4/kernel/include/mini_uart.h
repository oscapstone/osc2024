#ifndef	_MINI_UART_H
#define	_MINI_UART_H

#include <sys/types.h>
#include <stdint.h>

void uart_init ( void );
char uart_recv ( void );
void uart_send ( char c );
void uart_recv_string( char* buf );
uint32_t uart_recv_uint(void);
void uart_printf(char* fmt, ...);
void uart_send_num(int64_t, int, int);

void uart_irq_on();
void uart_irq_off();
void uart_irq_send(char*);
void uart_irq_read(char*);

void change_read_irq(int);
void change_write_irq(int);

void irq(int);

void recv_handler();
void write_handler();

#endif  /*_MINI_UART_H */
