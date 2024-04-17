#ifndef	_MINI_UART_H
#define	_MINI_UART_H

#include <stdint.h>

void uart_init ( void );
char uart_recv ( void );
void uart_send ( char c );
void uart_send_string(char* str);
void uart_hex(unsigned int d);

/* async part */
uint8_t uart_async_recv(void);
void uart_async_send(uint8_t c);
void uart_rx_handler();
void uart_tx_handler();
void uart_async_demo();

#endif  /*_MINI_UART_H */