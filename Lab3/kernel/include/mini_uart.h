#ifndef MINI_UART_H
#define MINI_UART_H

void uart_init(void);
char uart_recv(void);
void uart_send(char c);
void uart_send_string(const char* str);
void uart_send_hex(unsigned int data);
void uart_send_dec(unsigned int data);
void uart_send_space_level(unsigned int level);
void uart_enable_interrupt(void);
void uart_disable_interrupt(void);
void uart_handle_irq(void);
char uart_recv_async(void);
void uart_send_async(char);
void uart_send_string_async(const char* str);
void test_uart_async(void);

#endif /* MINI_UART_H */
