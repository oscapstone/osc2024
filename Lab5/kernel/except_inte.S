.section ".text"

.global from_el2_to_el1
.global set_exception_vector_table
.global exception_vector_table
.global child_fork_ret
.type child_fork_ret, @function

// save general registers to stack
.macro save_all
    sub sp, sp, 32 * 10
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

    mrs x0,  spsr_el1
    str x0,  [sp, 16 * 15 + 8]
    mrs x0,  elr_el1
    str x0,  [sp, 16 * 16]
    mrs x0,  sp_el0
    str x0,  [sp, 16 * 16 + 8]

    mov x0, sp    

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
    
    ldr x0,  [sp, 16 * 15 + 8]
    msr spsr_el1, x0
    ldr x0,  [sp, 16 * 16]
    msr elr_el1, x0
    ldr x0,  [sp, 16 * 16 + 8]
    msr sp_el0, x0

    ldp x0, x1, [sp ,16 * 0] 
    
    add sp, sp, 32 * 10
.endm

// disable interrupt -> level2 might have its own interrupt handler
from_el2_to_el1:
    mov x0, (1 << 31)       // EL1 uses aarch64
    msr hcr_el2, x0
    mov x0, 0x3c5           // EL1h (SPSel = 1) with interrupt disabled / 0x5 means EL1 with the highest level of exception handling in which the kernel or operating system typically operates
    msr spsr_el2, x0
    ldr x0, =0x3c000000
    msr sp_el1, x0
    msr elr_el2, lr
    eret                    // return to EL1

exception_handler:
    save_all
    bl exception_entry
    load_all
    eret

el1_irq_handler:
    save_all
    bl el1_irq_entry
    load_all
    eret

el0_irq_handler:
    save_all
    bl el0_irq_entry
    load_all
    eret

el0_sync_handler:
    save_all
    bl el0_sync_entry
    load_all
    eret

.align 11                   // vector table should be aligned to 0x800
exception_vector_table:
    b exception_handler     // branch to a handler function.
    .align 7                // entry size is 0x80, .align will pad 0
    b exception_handler
    .align 7
    b exception_handler
    .align 7
    b exception_handler
    .align 7

    b exception_handler
    .align 7
    b el1_irq_handler
    .align 7
    b exception_handler
    .align 7
    b exception_handler
    .align 7

    b el0_sync_handler
    .align 7
    b el0_irq_handler
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

set_exception_vector_table:
    adr x0, exception_vector_table
    msr vbar_el1, x0
    ret  

child_fork_ret:
    bl enable_core_timer
    bl enable_interrupt
    load_all
    eret