#ifndef _EXCEPTION_H_
#define _EXCEPTION_H_

#include "u_list.h"
#include "bcm2837/rpi_mmu.h"

#define SYSCALL(number, function) \
    case number:                  \
        function;                 \
        break;

// https://github.com/Tekki/raspberrypi-documentation/blob/master/hardware/raspberrypi/bcm2836/QA7_rev3.4.pdf p16
#define CORE0_INTERRUPT_SOURCE ((volatile unsigned int *)(PHYS_TO_KERNEL_VIRT(0x40000060)))
#define INTERRUPT_SOURCE_CNTPNSIRQ (1 << 1)
#define INTERRUPT_SOURCE_GPU (1 << 8)
#define IRQ_PENDING_1_AUX_INT (1 << 29)

typedef struct trapframe
{
    unsigned long x0;
    unsigned long x1;
    unsigned long x2;
    unsigned long x3;
    unsigned long x4;
    unsigned long x5;
    unsigned long x6;
    unsigned long x7;
    unsigned long x8;
    unsigned long x9;
    unsigned long x10;
    unsigned long x11;
    unsigned long x12;
    unsigned long x13;
    unsigned long x14;
    unsigned long x15;
    unsigned long x16;
    unsigned long x17;
    unsigned long x18;
    unsigned long x19;
    unsigned long x20;
    unsigned long x21;
    unsigned long x22;
    unsigned long x23;
    unsigned long x24;
    unsigned long x25;
    unsigned long x26;
    unsigned long x27;
    unsigned long x28;
    unsigned long x29;
    unsigned long x30;
    unsigned long spsr_el1;
    unsigned long elr_el1;
    unsigned long sp_el0;

} trapframe_t;

#define MEMFAIL_DATA_ABORT_LOWER 0b100100 // esr_el1
#define MEMFAIL_INST_ABORT_LOWER 0b100000 // EC, bits [31:26]

#define TF_LEVEL0 0b000100 // iss IFSC, bits [5:0]
#define TF_LEVEL1 0b000101
#define TF_LEVEL2 0b000110
#define TF_LEVEL3 0b000111

#define ESR_EL1_EC_SHIFT 26
#define ESR_EL1_EC_MASK 0x3F
#define ESR_EL1_EC_SVC64 0x15

typedef struct
{
    unsigned int iss : 25, // Instruction specific syndrome
        il : 1,            // Instruction length bit
        ec : 6;            // Exception class
} esr_el1_t;

void el1_interrupt_enable();
void el1_interrupt_disable();

void lock();
void unlock();

void el1h_sync_router(trapframe_t *tpf);
void el1h_irq_router(trapframe_t *tpf);
void el0_sync_router(trapframe_t *tpf);
void el0_irq_router(trapframe_t *tpf);

void invalid_exception_router(unsigned long long x0); // exception_handler.S

const char *get_exception_name(unsigned long esr_el1);


#endif /*_EXCEPTION_H_*/
