#ifndef _EXCEPTION_H_
#define _EXCEPTION_H_

#include "list.h"
#include "stdint.h"

#define UART_IRQ_PRIORITY 2
#define TIMER_IRQ_PRIORITY 1

typedef struct trapframe
{
    uint64_t x0;
    uint64_t x1;
    uint64_t x2;
    uint64_t x3;
    uint64_t x4;
    uint64_t x5;
    uint64_t x6;
    uint64_t x7;
    uint64_t x8;
    uint64_t x9;
    uint64_t x10;
    uint64_t x11;
    uint64_t x12;
    uint64_t x13;
    uint64_t x14;
    uint64_t x15;
    uint64_t x16;
    uint64_t x17;
    uint64_t x18;
    uint64_t x19;
    uint64_t x20;
    uint64_t x21;
    uint64_t x22;
    uint64_t x23;
    uint64_t x24;
    uint64_t x25;
    uint64_t x26;
    uint64_t x27;
    uint64_t x28;
    uint64_t x29;
    uint64_t x30;
    uint64_t spsr_el1;
    uint64_t elr_el1;
    uint64_t sp_el0;
} trapframe_t;

typedef struct irqtask
{
    struct list_head listhead;
    uint64_t priority;   // store priority (smaller number is more preemptive)
    void *task_function; // task function pointer
} irqtask_t;

void irqtask_add(void *task_function, uint64_t priority);
void irqtask_run(irqtask_t *the_task);
void irqtask_run_preemptive();
void irqtask_list_init();

// https://github.com/Tekki/raspberrypi-documentation/blob/master/hardware/raspberrypi/bcm2836/QA7_rev3.4.pdf p16
#define CORE0_INTERRUPT_SOURCE ((volatile unsigned int *)(0x40000060))

#define INTERRUPT_SOURCE_CNTPNSIRQ (1 << 1)
#define INTERRUPT_SOURCE_GPU (1 << 8)

#define IRQ_PENDING_1_AUX_INT (1 << 29)

void el1_interrupt_enable();
void el1_interrupt_disable();
void __lock_interrupt();
void __unlock_interrupt();

void el1h_irq_router(trapframe_t* tpf);
void el0_sync_router(trapframe_t* tpf);
void el0_irq_64_router(trapframe_t* tpf);

void invalid_exception_router(uint64_t x0); // exception_handler.S

#endif /*_EXCEPTION_H_*/
