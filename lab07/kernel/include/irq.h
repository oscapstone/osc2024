#ifndef __IRQ_H__
#define __IRQ_H__

#define EL1_IRQ_TIMER_PRIORITY          0x1
#define EL1_IRQ_UART_PRIORITY           0x2 

struct irq_task_struct;

void task_head_init();
void test_func();

#endif