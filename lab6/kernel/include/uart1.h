#ifndef _UART1_H_
#define _UART1_H_

void uart_init();
void uart_flush_FIFO();
char uart_recv();
void uart_send(unsigned int c);
void uart_puts(char *fmt, ...);
char uart_async_recv();
void uart_async_send(char c);
void uart_interrupt_enable();
void uart_interrupt_disable();
void uart_r_irq_handler();
void uart_w_irq_handler();

int is_uart_rx_buffer_full();
int is_uart_tx_buffer_full();

#endif /*_UART1_H_*/
