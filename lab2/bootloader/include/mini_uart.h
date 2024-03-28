#ifndef	_mini_UART_H_
#define	_mini_UART_H_

void uart_init();
char uart_recv();
void uart_send(unsigned int c);
void uart_puts(char* str);
void uart_hex(unsigned int d);
char uart_get();
void put_int(int num);


#endif /*_mini_UART_H_*/
