#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__

#include <stdint.h>

#define IRQ_PEND1_REG (volatile unsigned int*)0x3f00b204
#define IRQs1 (volatile unsigned int*)0x3f00b210 // 0x3f00b000+0x210
#define DISABLE_IRQs1 	 (volatile unsigned int*)0x3f00b21c

#define CORE0_TIMER_IRQ_CTRL 0x40000040 //?

#define CORE0_IRQ_SOURCE (volatile unsigned int*)0x40000060

typedef struct irq_task{
  uint32_t priority;
  int (*callback)(void);
  struct irq_task *next;
}irq_task_t;

extern irq_task_t* irq_task_head;
int add_irq_task(int (*callback)(void), uint64_t priority);

int irq_entry();
int lower_irq_entry();

void enable_irq();
void disable_irq();
void mini_uart_irq_enable();
void mini_uart_irq_disable();
void core_timer_enable();
void core_timer_disable();

// int core_timer_handler();
int timer_irq_handler();
int mini_uart_irq_handler();

int uart_rx_handler();
int uart_tx_handler();

#endif