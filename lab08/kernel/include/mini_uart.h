#ifndef _MINI_UART_H
#define _MINI_UART_H

#define BUFF_SIZE 256

void uart_init();
char uart_recv();
void uart_send(const char c);
void uart_send_string(const char* str);

void enable_uart_interrupt();
void disable_uart_interrupt();
void enable_uart_recv_interrupt();
void disable_uart_recv_interrupt();
void enable_uart_trans_interrupt();
void disable_uart_trans_interrupt();

void uart_buff_init();

void async_uart_puts(char* buff);
int async_uart_gets(char* buff, int size);

void async_uart_read_handler();
void async_uart_write_handler();

#endif