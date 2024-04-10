#ifndef	_UART1_H_
#define	_UART1_H_

void uart_init();
char uart_getc();
char uart_recv();
void uart_send(unsigned int c);
int  uart_puts(char* fmt, ...);
char uart_async_getc();
void uart_async_putc(char c);
void uart_2hex(unsigned int d);

void uart_interrupt_enable();
void uart_interrupt_disable();
void uart_interrupt_handler();
#endif /*_UART1_H_*/
