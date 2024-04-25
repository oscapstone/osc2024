#ifndef __IRQ_H__
#define __IRQ_H__

#include <lib/stdlib.h>

// el1 IRQ can be disable to protect the critical sections
void el1_enable_interrupt();
void el1_disable_interrupt();

// GPU IRQ handle is used to handle the GPU IRQ such as UART IRQ
void gpu_irq_handler();

// Core timer IRQ handle is used to handle the core timer IRQ
void core_timer_enable();
void core_timer_disable();
void core_timer_irq_handler();
uint64_t timer_get_uptime();

// IRQ utils
uint32_t is_core_timer_irq();
uint32_t is_gpu_irq();

uint32_t is_aux_irq();
uint32_t is_uart_rx_irq();
uint32_t is_uart_tx_irq();

const char *get_exception_message(unsigned long type);

#endif
