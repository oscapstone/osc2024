#ifndef	_P_MINI_UART_H
#define	_P_MINI_UART_H

#include "peripherals/base.h"
#define MAX_BUFFER_SIZE 256u

/* Synchronous, shell blocks the execution by busy polling the UART */
void uart_send ( char c );
char uart_recv ( void );
void uart_send_string(char* str);
void uart_send_hex(unsigned int d);
void uart_init ( void );

/* Asynchronous, interrupt handler, r/w buffer */
extern char read_buffer[MAX_BUFFER_SIZE];
extern char write_buffer[MAX_BUFFER_SIZE];
extern int r_head, r_tail;
extern int w_head, w_tail;

void async_uart_init();
void enable_uart_tx_interrupt();
void disable_uart_tx_interrupt();
void enable_uart_rx_interrupt();
void disable_uart_rx_interrupt();

void async_uart_send(char c);
void async_uart_send_string(char* str);
char async_uart_recv (void);

void uart_irq_handler();
void test_async();

#endif  /*_P_MINI_UART_H */
