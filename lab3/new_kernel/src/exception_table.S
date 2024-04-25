#include "exception_table.h"
// 保存中斷現場
.macro handle_invalid_entry type
    kernel_entry
    mov x0, #\type
    mrs x1, esr_el1
    mrs x2, elr_el1
    bl show_exception_status
    bl not_implemented
    kernel_exit
.endm

.macro ventry label
.align 7 // entry size is 0x80, .align will pad 0
    b \label
.endm

.macro kernel_entry

    sub sp, sp, 32 * 9
    stp x0, x1, [sp ,16 * 0]  // store pair of registers
    stp x2, x3, [sp ,16 * 1]
    stp x4, x5, [sp ,16 * 2]
    stp x6, x7, [sp ,16 * 3]
    stp x8, x9, [sp ,16 * 4]
    stp x10, x11, [sp ,16 * 5]
    stp x12, x13, [sp ,16 * 6]
    stp x14, x15, [sp ,16 * 7]
    stp x16, x17, [sp ,16 * 8]
    stp x18, x19, [sp ,16 * 9]
    stp x20, x21, [sp ,16 * 10]
    stp x22, x23, [sp ,16 * 11]
    stp x24, x25, [sp ,16 * 12]
    stp x26, x27, [sp ,16 * 13]
    stp x28, x29, [sp ,16 * 14]
    str x30, [sp, 16 * 15]
    // information for nested interrupt
    mrs x0, spsr_el1
    str x0, [sp, 16 * 16]
    mrs x0, elr_el1
    str x0, [sp, 16 * 17]
    ldp x0, x1, [sp ,16 * 0]  // restore x0 instead of elr_el1
.endm

.macro kernel_exit

    ldp x0, x1, [sp ,16 * 0]
    ldp x2, x3, [sp ,16 * 1]
    ldp x4, x5, [sp ,16 * 2]
    ldp x6, x7, [sp ,16 * 3]
    ldp x8, x9, [sp ,16 * 4]
    ldp x10, x11, [sp ,16 * 5]
    ldp x12, x13, [sp ,16 * 6]
    ldp x14, x15, [sp ,16 * 7]
    ldp x16, x17, [sp ,16 * 8]
    ldp x18, x19, [sp ,16 * 9]
    ldp x20, x21, [sp ,16 * 10]
    ldp x22, x23, [sp ,16 * 11]
    ldp x24, x25, [sp ,16 * 12]
    ldp x26, x27, [sp ,16 * 13]
    ldp x28, x29, [sp ,16 * 14]
    ldr x30, [sp, 16 * 15]
    // information for nested interrupt
    ldr x0, [sp, 16 * 16]
    msr spsr_el1,x0
    ldr x0, [sp, 16 * 17]
    msr elr_el1, x0
    ldp x0, x1, [sp ,16 * 0]  // restore x0 instead of elr_el1
    add sp, sp, 32 * 9
    ERET
.endm

.section ".text"

.align 11 //vector table should be aligned to 0x800 which is 2 to the power of 11
.global exception_table
exception_table:

    //Exception vector in EL1t mode
    ventry sync_invalid_el1t //Synchronous EL1t
    ventry irq_invalid_el1t // IRQ EL1t
    ventry fiq_invalid_el1t // FIQ EL1t
    ventry error_invalid_el1t //SError EL1t

    //Exception vector in EL1h mode
    ventry sync_exc_handler //Synchronous EL1h
    ventry irq_exc_handler // IRQ EL1h
    ventry fiq_invalid_el1h // FIQ EL1h
    ventry error_invalid_el1h //SError EL1h

    //Exception vector in 64 bits EL0 mode
    ventry sync_exc_handler
    ventry irq_exc_handler //
    ventry fiq_invalid_el0_64
    ventry error_invalid_el0_64

    //Exception vector in 32 bits EL0 mode
    ventry sync_invalid_el0_32
    ventry irq_invalid_el0_32
    ventry fiq_invalid_el0_32
    ventry error_invalid_el0_32

sync_invalid_el1t:
    handle_invalid_entry  SYNC_INVALID_EL1t

irq_invalid_el1t:
    handle_invalid_entry  IRQ_INVALID_EL1t

fiq_invalid_el1t:
    handle_invalid_entry  FIQ_INVALID_EL1t

error_invalid_el1t:
    handle_invalid_entry  ERROR_INVALID_EL1t

sync_invalid_el1h:
    handle_invalid_entry  SYNC_INVALID_EL1h

fiq_invalid_el1h:
    handle_invalid_entry  FIQ_INVALID_EL1h

error_invalid_el1h:
    handle_invalid_entry  ERROR_INVALID_EL1h

sync_invalid_el0_64:
    handle_invalid_entry  SYNC_INVALID_EL0_64

irq_invalid_el0_64:
    handle_invalid_entry  IRQ_INVALID_EL0_64

fiq_invalid_el0_64:
    handle_invalid_entry  FIQ_INVALID_EL0_64

error_invalid_el0_64:
    handle_invalid_entry  ERROR_INVALID_EL0_64

sync_invalid_el0_32:
    handle_invalid_entry  SYNC_INVALID_EL0_32

irq_invalid_el0_32:
    handle_invalid_entry  IRQ_INVALID_EL0_32

fiq_invalid_el0_32:
    handle_invalid_entry  FIQ_INVALID_EL0_32

error_invalid_el0_32:
    handle_invalid_entry  ERROR_INVALID_EL0_32

sync_exc_handler:
    kernel_entry
    mrs x0, SPSR_EL1
    mrs x1, elr_el1
    mrs x2, esr_el1
    bl  sync_exc_router
    kernel_exit

irq_exc_handler:
    kernel_entry
    bl  irq_exc_router  //
    kernel_exit
