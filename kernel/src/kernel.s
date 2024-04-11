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
    msr DAIFSet, #0xf
    sub sp, sp, 16
    stp x0, x1, [sp]
    stp x30, x31, [sp, 16]
    mov x0, 0
    bl _loop // branch to a handler function.
    ldp x0, x1, [sp]
    ldp x30, x31, [sp, 16]
    add sp, sp, 16
    msr DAIFClr, #0xf
    eret

    .align 7
    msr DAIFSet, #0xf // entry size is 0x80, .align will pad 0
    sub sp, sp, 16
    stp x0, x1, [sp]
    stp x30, x31, [sp, 16]
    mov x0, 1
    bl _loop
    ldp x0, x1, [sp]
    ldp x30, x31, [sp, 16]
    add sp, sp, 16
    msr DAIFClr, #0xf
    eret

    .align 7
    msr DAIFSet, #0xf
    sub sp, sp, 16
    stp x0, x1, [sp]
    stp x30, x31, [sp, 16]
    mov x0, 2
    bl _loop
    ldp x0, x1, [sp]
    ldp x30, x31, [sp, 16]
    add sp, sp, 16
    msr DAIFClr, #0xf
    eret

    .align 7
    msr DAIFSet, #0xf
    sub sp, sp, 16
    stp x0, x1, [sp]
    stp x30, x31, [sp, 16]
    mov x0, 3
    bl _loop
    ldp x0, x1, [sp]
    ldp x30, x31, [sp, 16]
    add sp, sp, 16
    msr DAIFClr, #0xf
    eret


    .align 7
    msr DAIFSet, #0xf
    sub sp, sp, 16
    stp x0, x1, [sp]
    stp x30, x31, [sp, 16]
    mov x0, 4
    bl exception_handler
    ldp x0, x1, [sp]
    ldp x30, x31, [sp, 16]
    add sp, sp, 16
    msr DAIFClr, #0xf
    eret

    .align 7
    msr DAIFSet, #0xf
    sub sp, sp, 16
    stp x0, x1, [sp]
    stp x30, x31, [sp, 16]
    mov x0, 5
    bl exception_handler
    ldp x0, x1, [sp]
    ldp x30, x31, [sp, 16]
    add sp, sp, 16
    msr DAIFClr, #0xf
    eret

    .align 7
    msr DAIFSet, #0xf
    sub sp, sp, 16
    stp x0, x1, [sp]
    stp x30, x31, [sp, 16]
    mov x0, 6
    bl _loop
    ldp x0, x1, [sp]
    ldp x30, x31, [sp, 16]
    add sp, sp, 16
    msr DAIFClr, #0xf
    eret

    .align 7
    msr DAIFSet, #0xf
    sub sp, sp, 16
    stp x0, x1, [sp]
    stp x30, x31, [sp, 16]
    mov x0, 7
    bl _loop
    ldp x0, x1, [sp]
    ldp x30, x31, [sp, 16]
    add sp, sp, 16
    msr DAIFClr, #0xf
    eret


    .align 7
    msr DAIFSet, #0xf
    sub sp, sp, 16
    stp x0, x1, [sp]
    stp x30, x31, [sp, 16]
    mov x0, 8
    bl exception_handler
    ldp x0, x1, [sp]
    ldp x30, x31, [sp, 16]
    add sp, sp, 16
    msr DAIFClr, #0xf
    eret

    .align 7
    msr DAIFSet, #0xf
    sub sp, sp, 16
    stp x0, x1, [sp]
    stp x30, x31, [sp, 16]
    mov x0, 9
    bl exception_handler
    ldp x0, x1, [sp]
    ldp x30, x31, [sp, 16]
    add sp, sp, 16
    msr DAIFClr, #0xf
    eret

    .align 7
    msr DAIFSet, #0xf
    sub sp, sp, 16
    stp x0, x1, [sp]
    stp x30, x31, [sp, 16]
    mov x0, 10
    bl _loop
    ldp x0, x1, [sp]
    ldp x30, x31, [sp, 16]
    add sp, sp, 16
    msr DAIFClr, #0xf
    eret

    .align 7
    msr DAIFSet, #0xf
    sub sp, sp, 16
    stp x0, x1, [sp]
    stp x30, x31, [sp, 16]
    mov x0, 11
    bl _loop
    ldp x0, x1, [sp]
    ldp x30, x31, [sp, 16]
    add sp, sp, 16
    msr DAIFClr, #0xf
    eret


    .align 7
    msr DAIFSet, #0xf
    sub sp, sp, 16
    stp x0, x1, [sp]
    stp x30, x31, [sp, 16]
    mov x0, 12
    bl _loop
    ldp x0, x1, [sp]
    ldp x30, x31, [sp, 16]
    add sp, sp, 16
    msr DAIFClr, #0xf
    eret

    .align 7
    msr DAIFSet, #0xf
    sub sp, sp, 16
    stp x0, x1, [sp]
    stp x30, x31, [sp, 16]
    mov x0, 13
    bl _loop
    ldp x0, x1, [sp]
    ldp x30, x31, [sp, 16]
    add sp, sp, 16
    msr DAIFClr, #0xf
    eret

    .align 7
    msr DAIFSet, #0xf
    sub sp, sp, 16
    stp x0, x1, [sp]
    stp x30, x31, [sp, 16]
    mov x0, 14
    bl _loop
    ldp x0, x1, [sp]
    ldp x30, x31, [sp, 16]
    add sp, sp, 16
    msr DAIFClr, #0xf
    eret

    .align 7
    msr DAIFSet, #0xf
    sub sp, sp, 16
    stp x0, x1, [sp]
    stp x30, x31, [sp, 16]
    mov x0, 15
    bl _loop
    ldp x0, x1, [sp]
    ldp x30, x31, [sp, 16]
    add sp, sp, 16
    msr DAIFClr, #0xf
    eret
    .align 7
    msr DAIFSet, #0xf

msr DAIFClr, #0xf
