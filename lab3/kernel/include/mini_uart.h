#ifndef	_MINI_UART_H
#define	_MINI_UART_H

void uart_init();
char uart_getc();
char uart_recv();
char uart_async_getc();
void uart_async_send(char c);
void uart_send(unsigned int c);
int  uart_puts(char* fmt, ...);
void uart_2hex(unsigned int d);
void uart_flush_FIFO();
void uart_interrupt_enable();


#endif  /*_MINI_UART_H */
