#ifndef _UART1_H_
#define _UART1_H_

void uart_init();
void uart_flush_FIFO();
char uart_recv();
void uart_send(unsigned int c);

void uart_r_irq_handler();
void uart_w_irq_handler();
char uart_async_recv();
void uart_async_send(char c);

int  uart_sendlinek(char* fmt, ...);

#endif /*_UART1_H_*/
