#ifndef	_MINI_UART_H
#define	_MINI_UART_H

void uart_init(void);
char uart_recv(void);
void uart_send(char c);
void uart_send_string(char* str);
void uart_send_string_int2hex(unsigned long value);
void uart_irq_handler(void);
void uart_enable_interrupt(void);
void uart_disable_interrupt(void);
void irq_uart_rx_exception(void);
void irq_uart_tx_exception(void);
char uart_async_recv(void);
void uart_async_send_string(const char* str);

#endif  /*_MINI_UART_H */