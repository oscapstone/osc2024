#include "reg.h"

.global from_el2_to_el1
from_el2_to_el1:
    ldr x1, =0
    msr sctlr_el1, x1 // disable MMU

    mov x0, #(1 << 31) // EL1 uses aarch64
    msr hcr_el2, x0

    // turn on the accessibility to SIMD and floating registers
    ldr x1, =0x300000 // 0b11 << 20
    msr cpacr_el1, x1

    mov x0, 0x3c5 // EL1h (SPSel = 1) with interrupt disabled
    msr spsr_el2, x0

    msr elr_el2, lr

    /* After eret, sp will be setup accordinge to sp_el1 */
    ldr x0, =_start
    msr sp_el1, x0

    eret // return to EL1

/* From el1 to el0 */
.global move_to_user_mode
move_to_user_mode:
    mov x0, #0x70000
    msr sp_el0, x0

    mov x1, #0x0 // same effect as `ldr x1, =0`
    msr spsr_el1, x1

    mov x1, lr
    and x1, x1, #0xffffffffffff // clear the high 16 bits for virtual address
    msr elr_el1, x1

    eret // return to EL0

// save general registers to stack
.macro save_all
    sub sp, sp, 34 * 8
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

    /* Lab4: save sp_el0, spsr_el1, elr_el1 for user task */
    mrs x21, sp_el0
    stp x30, x21, [sp, 16 * 15]

    mrs x21, spsr_el1
    mrs x22, elr_el1
    stp x21, x22, [sp, 16 * 16]
.endm

// load general registers from stack
.macro load_all
    /* Lab4: restore sp_el0, spsr_el1, elr_el1 for user task. */
    ldp x30, x21, [sp, 16 * 15]
    msr sp_el0, x21

    ldp x21, x22, [sp, 16 * 16]
    msr spsr_el1, x21
    msr elr_el1, x22
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
    add sp, sp, 34 * 8
.endm

.align 11 // vector table should be aligned to 0x800
.global exception_vector_table
exception_vector_table:
    /* Exception from the current EL while using SP_EL0 */
    .align 7 // synchronous
    b non_handler
    .align 7 // irq
    b non_handler
    .align 7 // fiq
    b non_handler
    .align 7 // serr
    b non_handler

    /* Exception from the current EL while using SP_ELx (x = 1 or 2) */
    .align 7 // synchronous
    b exception_handler
    .align 7 // irq
    b irq_handler
    .align 7 // fiq
    b non_handler
    .align 7 // serr
    b non_handler

    /* Exception from a lower Exception level and at least one lower Exception level uses AArch64. User program svc here */
     .align 7 // synchronous
    b exception_handler
    .align 7 // irq
    b irq_handler
    .align 7 // fiq
    b non_handler
    .align 7 // serr
    b non_handler

    /* Exception from a lower Exception level and all lower Exception levels use AArch32 */
    .align 7 // synchronous
    b non_handler
    .align 7 // irq
    b non_handler
    .align 7 // fiq
    b non_handler
    .align 7 // serr
    b non_handler

.global set_exception_vector_table
set_exception_vector_table:
    adr x0, exception_vector_table
    msr vbar_el1, x0
    ret

/* Handle synchronous exception and SVC */
exception_handler:
    save_all
    mrs     x0, esr_el1
    mrs     x1, elr_el1
    mrs     x2, spsr_el1
    mrs     x3, far_el1
    mov     x4, sp // for system call's trapframe
    bl svc_handler
    load_all
    eret

irq_handler:
    save_all
    mrs     x0, esr_el1
    mrs     x1, elr_el1
    mrs     x2, spsr_el1
    mrs     x3, far_el1
    //mrs     x4, sp_el0
    bl irq_router
    load_all
    eret

non_handler:
    b non_handler

.global exit_kernel
exit_kernel:
    load_all
    eret

.global disable_interrupt
disable_interrupt:
    msr daifset, #0xf
    ret

.global enable_interrupt
enable_interrupt:
    msr daifclr, #0xf
    ret

.global enable_irq
enable_irq:
    msr daifclr, #0x2
    ret

.global disable_irq
disable_irq:
    msr daifset, #0x2
    ret