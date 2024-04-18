#ifndef _INTERRUPT_H
#define _INTERRUPT_H

#include "peripherals/mmio.h"
#include "types.h"

#define CORE0_INT_SRC     \
  ((volatile unsigned int \
        *)0x40000060)  // The interrupt source register which
                       // shows what the source bits are for IRQ/FIQ
#define CORE_INT_SRC_TIMER (1 << 1)
#define CORE_INT_SRC_GPU (1 << 8)

#define IRQ_PENDING_1    \
  (volatile unsigned int \
       *)(MMIO_BASE + 0x0000B204)        // IRQ pending source 31:0
                                         // (IRQ table in the BCM2837 document)
#define IRQ_PENDING_1_AUX_INT (1 << 29)  // bit29: AUX INT
#define ENABLE_IRQS_1                    \
  ((volatile unsigned int *)(MMIO_BASE + \
                             0x0000B210))  // Set to enable IRQ source 31:0 (IRQ
                                           // table in the BCM2837 document)

typedef struct trapframe_t {
  uint64_t x[31];  // general registers from x0 ~ x30
  uint64_t sp_el0;
  uint64_t elr_el1;
  uint64_t spsr_el1;
} trapframe_t;

void el0_64_sync_interrupt_handler();
void el0_64_irq_interrupt_handler();
void el1h_irq_interrupt_handler();
void fake_long_handler();
void OS_enter_critical();
void OS_exit_critical();
extern void enable_interrupt();
extern void disable_interrupt();
extern void core_timer_enable();
extern void core_timer_disable();
extern void core0_timer_interrupt_enable();
extern void core0_timer_interrupt_disable();
extern void set_core_timer_int(uint64_t s);
extern void set_core_timer_int_sec(uint32_t s);

#endif