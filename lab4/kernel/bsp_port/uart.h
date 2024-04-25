#ifndef _UART_H
#define _UART_H

void uart_init();
void uart_send(unsigned int c);
char uart_getc();
void uart_puts(const char *s);
void uart_hex(unsigned int d);

void uart_enable_tx_interrupt();
void uart_disable_tx_interrupt();
void uart_enable_rx_interrupt();
void uart_disable_rx_interrupt();
void uart_tx_irq_handler();
void uart_rx_irq_handler();
void uart_async_read(char *buf);
void uart_async_write(const char *s);

#endif
