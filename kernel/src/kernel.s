.section ".text._start_kernel"

.global _start_kernel

_start_kernel:
    mrs x0, CurrentEL
    and x0, x0, #0xc
    cmp x0, #0b1000
    bne _loop

    bl from_el2_to_el1

    adr x0, exception_vector_table
    msr vbar_el1, x0

    b _start_rust

from_el2_to_el1:
    mov x0, (1 << 31) // EL1 uses aarch64
    msr hcr_el2, x0
    mov x0, 0x3c5 // EL1h (SPSel = 1) with interrupt disabled
    msr spsr_el2, x0
    msr elr_el2, lr
    mov x0, sp
    msr sp_el1, x0
    eret // return to EL1

_loop:
    b _loop

.align 11 // vector table should be aligned to 0x800
.global exception_vector_table
exception_vector_table:
    .align 7
    sub sp, sp, 8
    stp x30, x31, [sp]
    bl exception_handler // branch to a handler function.
    ldp x30, x31, [sp]
    add sp, sp, 8
    eret

    .align 7 // entry size is 0x80, .align will pad 0
    sub sp, sp, 8
    stp x30, x31, [sp]
    bl exception_handler
    ldp x30, x31, [sp]
    add sp, sp, 8
    eret

    .align 7
    sub sp, sp, 8
    stp x30, x31, [sp]
    bl exception_handler
    ldp x30, x31, [sp]
    add sp, sp, 8
    eret

    .align 7
    sub sp, sp, 8
    stp x30, x31, [sp]
    bl exception_handler
    ldp x30, x31, [sp]
    add sp, sp, 8
    eret


    .align 7
    sub sp, sp, 8
    stp x30, x31, [sp]
    bl exception_handler
    ldp x30, x31, [sp]
    add sp, sp, 8
    eret

    .align 7
    sub sp, sp, 8
    stp x30, x31, [sp]
    bl exception_handler
    ldp x30, x31, [sp]
    add sp, sp, 8
    eret

    .align 7
    sub sp, sp, 8
    stp x30, x31, [sp]
    bl exception_handler
    ldp x30, x31, [sp]
    add sp, sp, 8
    eret

    .align 7
    sub sp, sp, 8
    stp x30, x31, [sp]
    bl exception_handler
    ldp x30, x31, [sp]
    add sp, sp, 8
    eret


    .align 7
    sub sp, sp, 8
    stp x30, x31, [sp]
    bl exception_handler
    ldp x30, x31, [sp]
    add sp, sp, 8
    eret

    .align 7
    sub sp, sp, 8
    stp x30, x31, [sp]
    bl exception_handler
    ldp x30, x31, [sp]
    add sp, sp, 8
    eret

    .align 7
    sub sp, sp, 8
    stp x30, x31, [sp]
    bl exception_handler
    ldp x30, x31, [sp]
    add sp, sp, 8
    eret

    .align 7
    sub sp, sp, 8
    stp x30, x31, [sp]
    bl exception_handler
    ldp x30, x31, [sp]
    add sp, sp, 8
    eret


    .align 7
    sub sp, sp, 8
    stp x30, x31, [sp]
    bl exception_handler
    ldp x30, x31, [sp]
    add sp, sp, 8
    eret

    .align 7
    sub sp, sp, 8
    stp x30, x31, [sp]
    bl exception_handler
    ldp x30, x31, [sp]
    add sp, sp, 8
    eret

    .align 7
    sub sp, sp, 8
    stp x30, x31, [sp]
    bl exception_handler
    ldp x30, x31, [sp]
    add sp, sp, 8
    eret

    .align 7
    sub sp, sp, 8
    stp x30, x31, [sp]
    bl exception_handler
    ldp x30, x31, [sp]
    add sp, sp, 8
    eret
    .align 7
