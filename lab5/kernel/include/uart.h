#ifndef UART_H
#define UART_H

#include "gpio.h"
#include "irq.h"
#include "sprintf.h"

#define MAX_BUF_SIZE 0x400

#define INT_SRC_0
#define INT_AUX_RECV 0b00000100
#define INT_AUX_TRAN 0b00000010
#define INT_AUX_MASK 0b00000110

/* Auxilary mini UART registers */
#define AUX_ENABLE      ((volatile unsigned int*)(MMIO_BASE+0x00215004))
#define AUX_MU_IO       ((volatile unsigned int*)(MMIO_BASE+0x00215040))
#define AUX_MU_IER      ((volatile unsigned int*)(MMIO_BASE+0x00215044))
#define AUX_MU_IIR      ((volatile unsigned int*)(MMIO_BASE+0x00215048))
#define AUX_MU_LCR      ((volatile unsigned int*)(MMIO_BASE+0x0021504C))
#define AUX_MU_MCR      ((volatile unsigned int*)(MMIO_BASE+0x00215050))
#define AUX_MU_LSR      ((volatile unsigned int*)(MMIO_BASE+0x00215054))
#define AUX_MU_MSR      ((volatile unsigned int*)(MMIO_BASE+0x00215058))
#define AUX_MU_SCRATCH  ((volatile unsigned int*)(MMIO_BASE+0x0021505C))
#define AUX_MU_CNTL     ((volatile unsigned int*)(MMIO_BASE+0x00215060))
#define AUX_MU_STAT     ((volatile unsigned int*)(MMIO_BASE+0x00215064))
#define AUX_MU_BAUD     ((volatile unsigned int*)(MMIO_BASE+0x00215068))

extern int echo;

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