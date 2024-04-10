#ifndef _MINI_UART_H
#define _MINI_UART_H

#include "uart.h"
#include "type.h"
#include "exception.h"

#define IRQ_PENDING_1       ((volatile unsigned int*)(MMIO_BASE+0x0000b204))
#define ENABLE_IRQS_1		((volatile unsigned int*)(MMIO_BASE+0x0000b210))
#define DISABLE_IRQS_1      ((volatile unsigned int*)(MMIO_BASE+0x0000b21c))

void enable_uart_read_interrupt();
void disable_uart_read_interrupt();

void enable_uart_write_interrupt();
void disable_uart_write_interrupt();

void enable_uart_interrupt();
void disable_uart_interrupt();

void rx_interrupt_handler();
void tx_interrupt_handler();
void async_uart_handler();

uint32_t async_uart_gets(char*, uint32_t);
void async_uart_puts(char*);
void test_async_uart();

#endif