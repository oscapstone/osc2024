// save general registers to stack
.macro save_all
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
    // information for nested interrupt
    ldr x0, [sp, 16 * 16]
    msr spsr_el1,x0
    ldr x0, [sp, 16 * 17]
    msr elr_el1, x0
    ldp x0, x1, [sp ,16 * 0]  // restore x0 instead of elr_el1
    add sp, sp, 32 * 9
.endm

.macro    ventry    label
    .align    7   // entry should be aligned to 0x80 (2^7)
    b    \label
.endm

.align 11         // vector table should be aligned to 0x800 (2^11)
.global exception_vector_table

// exception_vector_table definition
// https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/entry.S#L367
exception_vector_table:
    //Exception from the current EL while using SP_EL0
    ventry    el1t_sync_invalid        // Synchronous EL1t
    ventry    el1t_irq_invalid         // IRQ EL1t
    ventry    el1t_fiq_invalid         // FIQ EL1t
    ventry    el1t_error_invalid       // Error EL1t

    //Exception from the current EL while using SP_ELx
    ventry    el1h_sync                // Synchronous EL1h
    ventry    el1h_irq                 // IRQ EL1h
    ventry    el1h_fiq_invalid         // FIQ EL1h
    ventry    el1h_error_invalid       // Error EL1h

    //Exception from a lower EL and at least one lower EL is AArch64
    ventry    el0_sync_64             // Synchronous 64-bit EL0
    ventry    el0_irq_64              // IRQ 64-bit EL0
    ventry    el0_fiq_invalid_64      // FIQ 64-bit EL0
    ventry    el0_error_invalid_64    // Error 64-bit EL0

    //Exception from a lower EL and at least all lower EL are AArch32
    ventry    el0_sync_invalid_32     // Synchronous 32-bit EL0
    ventry    el0_irq_invalid_32      // IRQ 32-bit EL0
    ventry    el0_fiq_invalid_32      // FIQ 32-bit EL0
    ventry    el0_error_invalid_32    // Error 32-bit EL0


el1t_sync_invalid:
    save_all
    mov x0,0
    bl invalid_exception_router
    load_all
    eret
el1t_irq_invalid:
    save_all
    mov x0,1
    bl invalid_exception_router
    load_all
    eret
el1t_fiq_invalid:
    save_all
    mov x0,2
    bl invalid_exception_router
    load_all
    eret
el1t_error_invalid:
    save_all
    mov x0,3
    bl invalid_exception_router
    load_all
    eret


el1h_sync:
    save_all
    mov x0,4
    bl invalid_exception_router
    load_all
    eret
el1h_irq:
    save_all
    mov x0,5
    bl invalid_exception_router
    load_all
    eret
el1h_fiq_invalid:
    save_all
    mov x0,6
    bl invalid_exception_router
    load_all
    eret
el1h_error_invalid:
    save_all
    mov x0,7
    bl invalid_exception_router
    load_all
    eret

el0_sync_64:
    save_all
    mov x0,8
    bl el0_sync_router
    load_all
    eret
el0_irq_64:
    save_all
    mov x0,9
    bl invalid_exception_router
    load_all
    eret
el0_fiq_invalid_64:
    save_all
    mov x0,10
    bl invalid_exception_router
    load_all
    eret
el0_error_invalid_64:
    save_all
    mov x0,11
    bl invalid_exception_router
    load_all
    eret


el0_sync_invalid_32:
    save_all
    mov x0,12
    bl invalid_exception_router
    load_all
    eret
el0_irq_invalid_32:
    save_all
    mov x0,13
    bl invalid_exception_router
    load_all
    eret
el0_fiq_invalid_32:
    save_all
    mov x0,14
    bl invalid_exception_router
    load_all
    eret
el0_error_invalid_32:
    save_all
    mov x0,15
    bl invalid_exception_router
    load_all
    eret
