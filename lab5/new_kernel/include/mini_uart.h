#ifndef	_mini_UART_H_
#define	_mini_UART_H_

void uart_init();
char uart_recv();
void uart_send(unsigned int c);
void uart_puts(char* str);
void uart_hex(unsigned int d);
char uart_get();
void put_int(int num);
void put_currentEL(void);
int  uart_sendline(char* fmt, ...);

void uart_flush_FIFO();
int is_uart_rx_buffer_full();
int is_uart_tx_buffer_full();

char getchar();
void putchar(char c);
void puts(const char *s);

char uart_async_getc();
void uart_async_putc(char c);
////////////////
void uart_interrupt_enable();
void uart_interrupt_disable();
void uart_r_irq_handler();
void uart_w_irq_handler();

#endif /*_mini_UART_H_*/
