/* save general registers to stack */
.macro save_all 
    sub sp, sp, 16 * 17
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
    mrs x10, spsr_el1   /* Saved Program Status Register (EL1) */
    mrs x11, elr_el1    /* Exception Link Register (EL1) */
    mrs x12, sp_el0
    stp x30, x10, [sp, 16 * 15]
    stp x11, x12, [sp, 16 * 16]
.endm

/* load general registers from stack */
.macro load_all
    ldp x30, x10, [sp, 16 * 15]
    ldp x11, x12, [sp, 16 * 16]
    msr spsr_el1, x10
    msr elr_el1, x11
    msr sp_el0, x12
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
    add sp, sp, 16 * 17
.endm

exception_handler:
    save_all
    mov x0, sp // pass the stack pointer to exception_entry
    bl exception_entry // defined in traps.c
    load_all
    eret

irq_handler:
    save_all
    mov x0, sp
    bl irq_entry // defined in irq.c
    load_all
    eret

invalid_exc_handler:
    save_all
    mrs x0, elr_el1     // Exception Link Register (EL1)
    mrs x1, esr_el1     // ESR_EL1, Exception Syndrome Register (EL1)
    mrs x2, spsr_el1    // Saved Program Status Register (EL1)
    bl invalid_entry    // defined in traps.c
    load_all
    eret

.global child_ret_from_fork
child_ret_from_fork:
    load_all // child's kernel stack (trap frame) 
    eret

.global sigreturn
sigreturn:
    mov x8, 139
    svc 0

.macro v_entry label
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
    v_entry invalid_exc_handler
    v_entry invalid_exc_handler
    v_entry invalid_exc_handler
    v_entry invalid_exc_handler
    // EL1 -> EL1 while using SP_EL1
    v_entry invalid_exc_handler
    v_entry irq_handler
    v_entry invalid_exc_handler
    v_entry invalid_exc_handler
    // EL0 (AArch64) -> EL1
    v_entry exception_handler
    v_entry irq_handler
    v_entry invalid_exc_handler
    v_entry invalid_exc_handler
    // EL0 (AArch32) -> EL1
    v_entry invalid_exc_handler
    v_entry invalid_exc_handler
    v_entry invalid_exc_handler
    v_entry invalid_exc_handler
