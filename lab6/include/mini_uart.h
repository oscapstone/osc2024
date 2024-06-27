#ifndef	_MINI_UART_H
#define	_MINI_UART_H

#define BUFFER_SIZE 1024

#include <stdint.h>

void uart_init ( void );
void uart_enable_interrupt( void );
void uart_disable_interrupt( void );
char uart_recv ( void );
void uart_send ( char c );
void uart_send_string(const char* str);
void uart_hex(uint64_t d);
void uart_irq_handler(void);
char uart_async_recv( void );
void uart_async_send_string(const char* buffer);

#endif  /*_MINI_UART_H */
