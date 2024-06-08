#ifndef __UART_H
#define __UART_H

#define MAX_SIZE 256

void uart_init();
char uart_read();
void uart_write(unsigned int c);
void uart_puts(char *s);
void uart_hex_upper_case(unsigned long long d);
void uart_hex_lower_case(unsigned long long d);
void uart_dec(unsigned long long d);

void enable_uart_rx_interrupt();
void disable_uart_rx_interrupt();
void enable_uart_tx_interrupt();
void disable_uart_tx_interrupt();
void enable_aux_interrupt();
void disable_aux_interrupt();

char uart_asyn_read();
void uart_asyn_write(unsigned int c);

extern char read_buf[MAX_SIZE];
extern char write_buf[MAX_SIZE];
extern int read_front;
extern int read_back;
extern int write_front;
extern int write_back;

#endif