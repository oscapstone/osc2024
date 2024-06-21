#ifndef _INTERRUPT_H
#define _INTERRUPT_H

#include "peripherals/mmio.h"
#include "types.h"
#include "vm_macro.h"

#define CORE0_INT_SRC             \
  ((volatile unsigned int         \
        *)((uint32_t)0x40000060 | \
           KERNEL_VIRT_BASE))  // The interrupt source register which
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

#define EXCEPTION_SVC 0x15
#define EXCEPTION_INSTR_ABORT_FR_LOW 0x20
#define EXCEPTION_INSTR_ABORT_FR_SAME 0x21
#define EXCEPTION_DATA_ABORT_FR_LOW 0x24
#define EXCEPTION_DATA_ABORT_FR_SAME 0x25

#define EC_MASK 0xFC000000
#define EC_shift 26

#define DFSC_MASK 0b111111
#define TASNSLATION_FAULT_L0 0b000100
#define TASNSLATION_FAULT_L1 0b000101
#define TASNSLATION_FAULT_L2 0b000110
#define TASNSLATION_FAULT_L3 0b000111
#define PERMISSION_FAULT_L0 0b001100
#define PERMISSION_FAULT_L1 0b001101
#define PERMISSION_FAULT_L2 0b001110
#define PERMISSION_FAULT_L3 0b001111

void el1h_sync_handler();
void el0_64_sync_handler();
void irq_interrupt_handler();
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