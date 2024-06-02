
#include "exception.h"

.macro save_all
    sub sp, sp, #32 * 8
    stp x0, x1, [sp ,16 * 0]
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
.endm

// load general registers from stack
.macro load_all
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
    add sp, sp, #32 * 8
.endm

.macro kernel_entry el
    save_all        // save all general register in kernel

    // save the current exception register
    sub     sp, sp, 2 * 16
    
    // if is user mode, save the user mode stack
    .if \el == 0
    mrs     x0, sp_el0
    str     x0, [sp, 16 * 0]
    .endif

    mrs     x0, elr_el1
    mrs     x1, spsr_el1
    stp     x0, x1, [sp, 16 * 1]

.endm

.macro kernel_exit el

    ldp     x8, x9, [sp, 16 * 1]
    msr     elr_el1, x8
    msr     spsr_el1, x9

    .if \el == 0
    ldr     x9, [sp, 16 * 0]
    msr     sp_el0, x9
    .endif

    add     sp, sp, 2 * 16

    load_all
.endm

.macro handle_invalid_entry type
    kernel_entry 1
    mov     x0, #\type
    mrs     x1, esr_el1
    mrs     x2, elr_el1
    mrs     x3, spsr_el1
    bl      show_invalid_entry_message
    bl      err_hang
    kernel_exit 1
    eret
.endm

.macro  ventry      label
    b   \label
.align  7
.endm

.align 11
.global exception_vector
exception_vector:
    ventry      sync_invalid_el1t
    ventry      irq_invalid_el1t
    ventry      fiq_invalid_el1t
    ventry      error_invalid_el1t

    ventry      handle_el1_sync
    ventry      handle_el1_irq
    ventry      fiq_invalid_el1h
    ventry      error_invalid_el1h

    ventry      handle_el0_64_sync
    ventry      irq_el0_handler
    ventry      fiq_invalid_el0_64
    ventry      error_invalid_el0_64

    ventry      sync_invalid_el0_32
    ventry      irq_invalid_el0_32
    ventry      fiq_invalid_el0_32
    ventry      error_invalid_el0_32

sync_invalid_el1t:
    handle_invalid_entry SYNC_INVALID_EL1t    
irq_invalid_el1t:
    handle_invalid_entry IRQ_INVALID_EL1t
fiq_invalid_el1t:
    handle_invalid_entry FIQ_INVALID_EL1t
error_invalid_el1t:
    handle_invalid_entry ERROR_INVALID_EL1t

handle_el1_sync:
    kernel_entry 1
    bl exception_sync_el1_handler // test
    kernel_exit 1
    eret

handle_el1_irq:
    kernel_entry 1
    bl      handle_irq
    kernel_exit 1
    eret

fiq_invalid_el1h:
    handle_invalid_entry FIQ_INVALID_EL1h
error_invalid_el1h:
    handle_invalid_entry ERROR_INVALID_EL1h

// handle user mode system call
handle_el0_64_sync:
    kernel_entry 0
    // give the handler register info
    mov     x0, sp
    bl exception_sync_el0_handler
    kernel_exit 0
    eret                // back to el0

irq_el0_handler:
    kernel_entry 0
    bl handle_irq
    kernel_exit 0
    eret                // back to el0
fiq_invalid_el0_64:
    handle_invalid_entry FIQ_INVALID_EL0_64
error_invalid_el0_64:
    handle_invalid_entry ERROR_INVALID_EL0_64

sync_invalid_el0_32:
    handle_invalid_entry SYNC_INVALID_EL0_32
irq_invalid_el0_32:
    handle_invalid_entry IRQ_INVALID_EL0_32
fiq_invalid_el0_32:
    handle_invalid_entry FIQ_INVALID_EL0_32
error_invalid_el0_32:
    handle_invalid_entry ERROR_INVALID_EL0_32

.global err_hang
err_hang:
    b       err_hang

.global set_exception_vector_table
set_exception_vector_table:
    adr     x0, exception_vector
    msr     vbar_el1, x0
    ret
