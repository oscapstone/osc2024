// .section .text.exception_vector_table
.align 11
.global exception_vector_table
exception_vector_table:
    b svc_handler
    .align 7
    b exception_handler
    .align 7
    b exception_handler
    .align 7
    b exception_handler
    .align 7
    b svc_handler
    .align 7
    b irq_handler
    .align 7
    b exception_handler
    .align 7
    b exception_handler
    .align 7
    b svc_handler
    .align 7
    b irq_handler
    .align 7
    b exception_handler
    .align 7
    b exception_handler
    .align 7
    b exception_handler
    .align 7
    b exception_handler
    .align 7
    b exception_handler
    .align 7
    b exception_handler
    .align 7

// save general registers to stack
.macro save_all
    sub sp, sp, 36 * 8
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
    stp x30, x31, [sp, 16 * 15]
    mrs x0, elr_el1
    mrs x1, sp_el0
    stp x0, x1, [sp, 16 * 16]
    mrs x0, spsr_el1
    stp x0, xzr, [sp, 16 * 17]
.endm

// load general registers from stack
.macro load_all
    ldp x0, x1, [sp, 16 * 17]
    msr spsr_el1, x0
    ldp x0, x1, [sp, 16 * 16]
    msr elr_el1, x0
    msr sp_el0, x1
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
    ldp x30, x31, [sp, 16 * 15]
    add sp, sp, 36 * 8
.endm

// Dummy handlers
.global exception_handler
    exception_handler:
    save_all
    bl disable_irq
    mov x0, sp
    bl exception_handler_rust
    bl enable_irq
    load_all
    eret

.global irq_handler
irq_handler:
    save_all
    bl disable_irq
    mov x0, sp
    bl irq_handler_rust
    bl enable_irq
    load_all
    eret

.global svc_handler
svc_handler:
    save_all
    bl disable_irq
    mov x0, sp
    bl svc_handler_rust
    bl enable_irq
    load_all
    eret

