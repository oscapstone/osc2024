#ifndef	_UART1_H_
#define	_UART1_H_

void uart_init();
char uart_getc();
char uart_recv();
void uart_send(unsigned int c);
int  uart_puts(char* fmt, ...);
void uart_2hex(unsigned int d);

#endif /*_UART1_H_*/
