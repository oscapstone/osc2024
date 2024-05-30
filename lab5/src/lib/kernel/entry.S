// save general registers to stack
.macro save_all
    sub sp, sp, 32 * 9
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

/* 
    // for nested interrupt
    mrs x0, spsr_el1
    str x0, [sp, 16 * 16]
    mrs x0, elr_el1
    str x0, [sp, 16 * 17]
    ldp x0, x1, [sp, 16 * 0] // restore x0, x1
*/

    // for nested interrupt
    mrs x0, spsr_el1
    str x0, [sp, 16 * 15 + 8]
    mrs x0, elr_el1
    str x0, [sp, 16 * 16]
    mrs x0, sp_el0
    str x0, [sp, 16 * 16 + 8]
    ldp x0, x1, [sp, 16 * 0] // restore x0, x1
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

/* 
    // for nested interrupt
    ldr x0, [sp, 16 * 16]
    msr spsr_el1, x0
    ldr x0, [sp, 16 * 17]
    msr elr_el1, x0
    ldp x0, x1, [sp, 16 * 0] // restore x0, x1
    add sp, sp, 32 * 9
*/

    // for nested interrupt
    ldr x0, [sp, 16 * 15 + 8]
    msr spsr_el1, x0
    ldr x0, [sp, 16 * 16]
    msr elr_el1, x0
    ldr x0, [sp, 16 * 16 + 8]
    msr sp_el0, x0
    ldp x0, x1, [sp, 16 * 0] // restore x0, x1
    add sp, sp, 32 * 9
.endm

/*
exception_handler:
    save_all
    bl exception_handler_c
    load_all
    eret
*/

invalid_handler:
    save_all
    load_all
    eret

el1h_sync_handler:
    save_all
    bl invalid_op_handler
    load_all
    eret

el1h_irq_handler:
    save_all
    mov x0, sp
    bl irq_handler_c
    load_all
    eret

el0_sync_handler:
    save_all
    mov x0, sp
    bl el0_sync_router
    load_all
    eret

el0_irq_handler:
    save_all
    mov x0, sp
    bl el0_irq_router
    load_all
    eret

.align 11 // vector table should be aligned to 0x800
.global exception_vector_table
exception_vector_table:
    b invalid_handler // branch to a handler function.
    .align 7 // entry size is 0x80, .align will pad 0
    b invalid_handler
    .align 7
    b invalid_handler
    .align 7
    b invalid_handler
    .align 7

    b el1h_sync_handler
    .align 7
    b el1h_irq_handler
    .align 7
    b invalid_handler
    .align 7
    b invalid_handler
    .align 7

    b el0_sync_handler
    .align 7
    b el0_irq_handler
    .align 7
    b invalid_handler
    .align 7
    b invalid_handler
    .align 7

    b invalid_handler
    .align 7
    b invalid_handler
    .align 7
    b invalid_handler
    .align 7
    b invalid_handler
    .align 7