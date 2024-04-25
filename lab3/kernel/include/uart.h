#ifndef UART_H
#define UART_H

#include "gpio.h"
#include "irq.h"
#include "sprintf.h"

#define MAX_BUF_SIZE 0x400
#define ENABLE_IRQS_1 ((volatile unsigned int*)(INT_BASE+0x210))
#define IRQ_PENDING_1 ((volatile unsigned int*)(INT_BASE+0x204))
#define IRQ_PENDING_1_AUX_INT (1 << 29)

#define INT_SRC_0
#define INT_AUX_RECV 0b00000100
#define INT_AUX_TRAN 0b00000010
#define INT_AUX_MASK 0b00000110



void init_idx();
void uart_init();
void uart_send(unsigned int c);
char uart_getc();
void uart_puts(char *s);
void uart_hex(unsigned int d);
void uart_dec(int d);
int uart_printf(char *fmt, ...);

void uart_async_putc(char c); 
char uart_async_getc();
int uart_async_printf(char *fmt, ...);

void enable_mini_uart_interrupt();
void enable_mini_uart_rx_interrupt();
void disable_mini_uart_rx_interrupt();
void enable_mini_uart_tx_interrupt();
void disable_mini_uart_tx_interrupt();

void uart_tx_interrupt_handler();
void uart_rx_interrupt_handler();
#endif