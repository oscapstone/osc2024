#ifndef _EXCEPTION_H_
#define _EXCEPTION_H_

#include "list.h"
#include "stdint.h"
#include "bcm2837/rpi_mmu.h"

#define UART_IRQ_PRIORITY 2
#define TIMER_IRQ_PRIORITY 1

#define MEMFAIL_DATA_ABORT_LOWER 0b100100 // esr_el1
#define MEMFAIL_INST_ABORT_LOWER 0b100000 // EC, bits [31:26]

#define TF_LEVEL0 0b000100 // iss IFSC, bits [5:0]
#define TF_LEVEL1 0b000101
#define TF_LEVEL2 0b000110
#define TF_LEVEL3 0b000111

#define PERMISSON_FAULT_LEVEL0 0b001100
#define PERMISSON_FAULT_LEVEL1 0b001101
#define PERMISSON_FAULT_LEVEL2 0b001110
#define PERMISSON_FAULT_LEVEL3 0b001111

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

typedef struct
{
    uint32_t iss : 25, // Instruction specific syndrome
        il : 1,        // Instruction length bit
        ec : 6;        // Exception class
} esr_el1_t;

void irqtask_add(void *task_function, uint64_t priority);
void irqtask_run(irqtask_t *the_task);
void irqtask_run_preemptive();
void irqtask_list_init();

// https://github.com/Tekki/raspberrypi-documentation/blob/master/hardware/raspberrypi/bcm2836/QA7_rev3.4.pdf p16
#define CORE0_INTERRUPT_SOURCE ((volatile unsigned int *)(_PHYS_TO_KERNEL_VIRT(0x40000060)))

#define INTERRUPT_SOURCE_CNTPNSIRQ (1 << 1)
#define INTERRUPT_SOURCE_GPU (1 << 8)

#define IRQ_PENDING_1_AUX_INT (1 << 29)

void el1_interrupt_enable();
void el1_interrupt_disable();
void __lock_interrupt();
void __unlock_interrupt();

void el1h_irq_router(trapframe_t *tpf);
void el0_sync_router(trapframe_t *tpf);
void el0_irq_64_router(trapframe_t *tpf);

void invalid_exception_router(uint64_t x0); // exception_handler.S

#endif /*_EXCEPTION_H_*/
