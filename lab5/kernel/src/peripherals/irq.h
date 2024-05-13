#pragma once

#include "base.h"
#include "bcm2837base.h"

// BCM2837 pg.113
enum vc_irqs {
    SYS_TIMER_1     = (1 << 1),
    SYS_TIMER_3     = (1 << 3),
    AUX_IRQ         = (1 << 29)
};

// BCM2837 pg.112
struct arm_irq_regs {
    REG32 irq_base_pending;
    REG32 irq_pending_1;
    REG32 irq_pending_2;
    REG32 fiq_control;
    REG32 irq_enable_1;
    REG32 irq_enable_2;
    REG32 irq_base_enable;
    REG32 irq_disable_1;
    REG32 irq_disable_2;
    REG32 irq_base_disable;
};

#define REGS_IRQ    ((struct arm_irq_regs *)(PBASE + 0x0000b200))

void show_invalid_entry_message(U32 type, U64 esr, U64 address, U64 spsr);
void enable_interrupt_controller();
void handle_irq();

// ARMv8 pg. 254 DAIF, mask and unmask the irq bit
void enable_interrupt();
void disable_interrupt();
void irq_restore(U64 flag);
U64 irq_disable();