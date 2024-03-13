// Load the address of a symbol into a register.
// The address is PC-relative and must lie within +/- 4GiB of the Program Counter.
.macro ADR_REL register, symbol
    adrp    \register, \symbol
    add     \register, \register, #:lo12:\symbol
.endm

.section .text._start

_start:
    // Read cpu id to x0
    mrs     x0, MPIDR_EL1
    and     x0, x0, #3              // asm_const is an unstable feature, write immediate constants directly.
    // Load the boot core id to x1
    ldr     x1, BOOT_CORE_ID
    cmp     x0, x1
    // Park it if it's not the boot core
    b.ne    .L_parking_loop

    ADR_REL x0, __bss_start
    ADR_REL x1, __bss_end_exclusive

.L_bss_init_loop:
    // Run it until the whole BSS have been gone through
    cmp     x0, x1
    b.eq    .L_prepare_rust
    // Store Pair of Register (post-index form)
    // 1. load 2 * 64-bit of zeros into x0
    // 2. add 16 to x0
    stp     xzr, xzr, [x0], #16
    b       .L_bss_init_loop

.L_prepare_rust:
    // Set the stack pointer
    ADR_REL x0, __boot_core_stack_end_exclusive
    mov     sp, x0

    // Jump to rust code
    b main

.L_parking_loop:
    wfe
    b       .L_parking_loop

.size   _start, . - _start
.type   _start, function
.global _start
