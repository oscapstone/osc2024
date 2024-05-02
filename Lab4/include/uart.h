#ifndef	_UART_H_
#define	_UART_H_

void uart_init();
char uart_recv();
void uart_send(char c);
int  uart_sendline(char* fmt, ...);
char uart_async_getc();
void uart_async_putc(char c);
int  uart_puts(char* fmt, ...);
void uart_2hex(unsigned int d);

void uart_interrupt_enable();
void uart_interrupt_disable();
void uart_r_irq_handler();
void uart_w_irq_handler();

#endif /*_UART_H_*/
