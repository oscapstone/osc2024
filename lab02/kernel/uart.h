#ifndef UART_H
#define UART_H


void mini_uart_init();
void mini_uart_send(char data);
char mini_uart_recv();
void mini_uart_gets(char *buffer, int size);



#endif
