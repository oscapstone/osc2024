/* save general registers to stack */
.macro save_all
    sub sp, sp, 32 * 8
    stp x0, x1, [sp, 16 * 0]
    stp x2, x3, [sp, 16 * 1]
    stp x4, x5, [sp, 16 * 2]
    stp x6, x7, [sp, 16 * 3]
    stp x8, x9, [sp, 16 * 4]
    stp x10, x11, [sp, 16 * 5]
    stp x12, x13, [sp, 16 * 6]
    stp x14, x15, [sp, 16 * 7]
    stp x16, x17, [sp, 16 * 8]
    stp x18, x19, [sp, 16 * 9]
    stp x20, x21, [sp, 16 * 10]
    stp x22, x23, [sp, 16 * 11]
    stp x24, x25, [sp, 16 * 12]
    stp x26, x27, [sp, 16 * 13]
    stp x28, x29, [sp, 16 * 14]
    str x30, [sp, 16 * 15]
.endm

/* load general registers from stack */
.macro load_all
    ldp x0, x1, [sp, 16 * 0]
    ldp x2, x3, [sp, 16 * 1]
    ldp x4, x5, [sp, 16 * 2]
    ldp x6, x7, [sp, 16 * 3]
    ldp x8, x9, [sp, 16 * 4]
    ldp x10, x11, [sp, 16 * 5]
    ldp x12, x13, [sp, 16 * 6]
    ldp x14, x15, [sp, 16 * 7]
    ldp x16, x17, [sp, 16 * 8]
    ldp x18, x19, [sp, 16 * 9]
    ldp x20, x21, [sp, 16 * 10]
    ldp x22, x23, [sp, 16 * 11]
    ldp x24, x25, [sp, 16 * 12]
    ldp x26, x27, [sp, 16 * 13]
    ldp x28, x29, [sp, 16 * 14]
    ldr x30, [sp, 16 * 15]
    add sp, sp, 32 * 8
.endm

exception_handler:
    save_all
    mrs x0, elr_el1
    mrs x1, esr_el1
    mrs x2, spsr_el1
    bl exception_entry // defined in traps.c
    load_all
    eret

irq_handler:
    save_all
    bl irq_entry // defined in irq.c
    load_all
    eret

invalid_exc_handler:
    save_all
    mrs x0, elr_el1
    mrs x1, esr_el1
    mrs x2, spsr_el1
    bl invalid_entry // defined in traps.c
    load_all
    eret

.macro ventry label
    b \label
.align 7
.endm

/* EL1 Exception Vector table
 * vector table should be aligned to 0x800
 * and each entry size is 0x80 */
.align 11
.global exception_vector_table
exception_vector_table:
    // EL1 -> EL1 while using SP_EL0
    ventry invalid_exc_handler
    ventry invalid_exc_handler
    ventry invalid_exc_handler
    ventry invalid_exc_handler
    // EL1 -> EL1 while using SP_EL1
    ventry invalid_exc_handler
    ventry irq_handler
    ventry invalid_exc_handler
    ventry invalid_exc_handler
    // EL0 (AArch64) -> EL1
    ventry exception_handler
    ventry invalid_exc_handler
    ventry invalid_exc_handler
    ventry invalid_exc_handler
    // EL0 (AArch32) -> EL1
    ventry invalid_exc_handler
    ventry invalid_exc_handler
    ventry invalid_exc_handler
    ventry invalid_exc_handler
