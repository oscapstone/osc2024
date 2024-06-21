#ifndef _EXECPTION_H_
#define _EXECPTION_H_

#define CORE0_INTERRUPT_SOURCE ((volatile unsigned int *)0x40000060)

#define INTERRUPT_SOURCE_CNTPNSIRQ (1<<1)
#define INTERRUPT_SOURCE_GPU (1<<8)

#define IRQ_PENDING_1_AUX_INT (1<<29)

typedef struct trapframe_t {
    unsigned long x[31]; // general purpose register
    // three reg that will be used to kernel mode -> user mode
    unsigned long spsr_el1;
    unsigned long elr_el1;
    unsigned long sp_el0;
} trapframe_t;

void el1_interrupt_enable();
void el1_interrupt_disable();

void core_timer_init();
void core_timer_enable();
void core_timer_disable();

void exception_handler_c();
void irq_exception_handler_c();
void user_exception_handler_c(trapframe_t* tf);

void irq_timer_exception();
void user_irq_timer_exception();
void irq_uart_rx_exception();
void irq_uart_tx_exception();

#endif