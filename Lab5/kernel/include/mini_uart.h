#ifndef MINI_UART_H
#define MINI_UART_H

#define UART_IRQ_PRIORITY 2

void uart_init(void);
char uart_recv(void);
void uart_send(char c);
void uart_send_string(const char *str);
void uart_send_hex(unsigned int data);
void uart_send_dec(unsigned int data);
void uart_send_space_level(unsigned int level);

void set_tx_interrupt(void);
void clear_tx_interrupt(void);

void set_rx_interrupt(void);
void clear_rx_interrupt(void);

void uart_enable_interrupt(void);
void uart_disable_interrupt(void);

char uart_recv_async(void);
void uart_send_async(char);
void uart_send_string_async(const char *str);

void uart_rx_handle_irq(void);
void uart_tx_handle_irq(void);

void test_uart_async(void);

void uart_printf(char *fmt, ...);

#endif /* MINI_UART_H */
