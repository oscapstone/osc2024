#ifndef MINI_UART_H
#define MINI_UART_H

void uart_init(void);
char uart_recv(void);
void uart_send(char c);
void uart_send_string(const char *str);

#endif /* MINI_UART_H */
