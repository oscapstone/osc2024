#ifndef _UART_H_
#define _UART_H_

void uart_init();               // initialize the device and maps it to the GPIO ports
void uart_send(unsigned int c); // send a character
char uart_getc();               // receive a character
char uart_getrawc();
void uart_puts(char *s);       // send a string
void uart_hex(unsigned int d); // send a hex number
void uart_dec(unsigned int d); // send a decimal number
int uart_printf(char *fmt, ...);

void uart_interrupt_enable();
void uart_interrupt_disable();
void uart_rx_interrupt_enable();
void uart_rx_interrupt_disable();
void uart_tx_interrupt_enable();
void uart_tx_interrupt_disable();
void uart_async_handler();
void uart_tx_handler();
void uart_rx_handler();
void uart_async_putc(char c);
char uart_async_getc();
void uart_clear_buffers();

#endif