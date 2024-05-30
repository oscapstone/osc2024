.section ".text"

.global _start

_start:
    mrs x1, mpidr_el1       // read the MPIDR_EL1 register; mrs = move from system register
    and x1, x1, #0x3
    cbz x1, cpu0            // cbz = compare and branch if zero; if x0 is zero, branch to cpu0

halt:
    wfe                     // wait for event
    b halt

cpu0:
    mov x21, x0
    bl from_el2_to_el1
    bl set_exception_vector_table

    ldr x1, =_start         // load the address of _start into x0
    mov sp, x1              // set the stack pointer to the address of _start
    ldr x1, =__bss_start    // load the address of __bss_start into x0
    ldr w2, =__bss_size

clear_bss:
    cbz w2, kernel_main
    str xzr, [x1], #8       // store zero to the address in x0, then increment x0 by 8
    sub w2, w2, #1
    cbnz w2, clear_bss

kernel_main:
    bl main                 // jump to C code
    b halt

from_el2_to_el1:
    mov x20, (1 << 31)       // EL1 uses aarch64
    msr hcr_el2, x20         // move x0 to the HCR_EL2 register
    mov x20, 0x3c5           // EL1h (SPSel = 1) with interrupt disabled
    msr spsr_el2, x20        // move x0 to the SPSR_EL2 register
    msr elr_el2, lr         // move the link register to the ELR_EL2 register
    eret                    // return to EL1


// save general registers to stack
.macro save_all
    sub sp, sp, 32 * 8
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

    // for nested interrupt
    mrs x0, spsr_el1
    str x0, [sp, 16 * 16]
    mrs x0, elr_el1
    str x0, [sp, 16 * 17]
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

    // for nested interrupt
    ldr x0, [sp, 16 * 16]
    msr spsr_el1, x0
    ldr x0, [sp, 16 * 17]
    msr elr_el1, x0
    ldp x0, x1, [sp, 16 * 0] // restore x0, x1

    add sp, sp, 32 * 8
.endm

exception_handler:
    save_all
    bl exception_handler_c
    load_all
    eret

invalid_handler:
    save_all
    // bl exception_handler_c
    load_all
    eret

irq_exception_handler:
    save_all
    bl irq_handler_c
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

    b invalid_handler
    .align 7
    b irq_exception_handler
    .align 7
    b invalid_handler
    .align 7
    b invalid_handler
    .align 7

    b exception_handler
    .align 7
    b irq_exception_handler
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

set_exception_vector_table:
    adr x0, exception_vector_table
    msr vbar_el1, x0
    ret
