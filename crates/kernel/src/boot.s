// Load the address of a symbol into a register.
// The address is PC-relative and must lie within +/- 4GiB of the Program Counter.
.macro ADR_REL register, symbol
    adrp    \register, \symbol
    add     \register, \register, #:lo12:\symbol
.endm

.section .text._start

_start:
    // Read cpu id to x1
    mrs     x1, MPIDR_EL1
    and     x1, x1, {CONST_CORE_ID_MASK}
    // Load the boot core id to x2
    ldr     x2, BOOT_CORE_ID
    cmp     x1, x2
    // Park it if it's not the boot core
    b.ne    .L_parking_loop

    ADR_REL x1, __bss_start
    ADR_REL x2, __bss_end_exclusive

.L_bss_init_loop:
    // Run it until the whole BSS have been gone through
    cmp     x1, x2
    b.eq    .L_prepare_rust
    // Store Pair of Register (post-index form)
    // 1. load 2 * 64-bit of zeros into x1
    // 2. add 16 to x1
    stp     xzr, xzr, [x1], #16
    b       .L_bss_init_loop

.L_prepare_rust:
    // Set the stack pointer
    ADR_REL x1, __boot_core_stack_end_exclusive
    mov     sp, x1

    // Jump to rust code
    b _start_rust

.L_parking_loop:
    wfe
    b       .L_parking_loop

.size   _start, . - _start
.type   _start, function
.global _start

// vim: ft=asm
