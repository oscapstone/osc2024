// Load the address of a symbol into a register.
// The address is PC-relative and must lie within +/- 4GiB of the Program Counter.
.macro ADR_REL register, symbol
    adrp    \register, \symbol
    add     \register, \register, #:lo12:\symbol
.endm

// Load the address of a symbol into a register.
// The address is an absolute address.
.macro ADR_ABS register, symbol
    movz    \register, #:abs_g2:\symbol
    movk    \register, #:abs_g1_nc:\symbol
    movk    \register, #:abs_g0_nc:\symbol
.endm

.section .text._start

_start:
    // Read cpu id to x0
    mrs     x0, MPIDR_EL1
    // asm_const is an unstable feature, write immediate constants directly.
    and     x0, x0, #3
    // Load the boot core id to x1
    ldr     x1, BOOT_CORE_ID
    cmp     x0, x1
    // Park it if it's not the boot core
    b.ne    .L_parking_loop

    // cf. crates/kernel/src/boot.s:20
    //   We should use absolute address here, because we tell the
    //   linker that we're going to place `_start` at 0x60000,
    //   but rpi3 will load the kernel at 0x80000.
    //   So we cannot use PC-relative address here.
    ADR_ABS x0, __bss_start
    ADR_ABS x1, __bss_end_exclusive

.L_bss_init_loop:
    // Run it until the whole BSS have been gone through
    cmp     x0, x1
    b.eq    .L_relocate_binary
    // Store Pair of Register (post-index form)
    // 1. load 2 * 64-bit of zeros into x0
    // 2. add 16 to x0
    stp     xzr, xzr, [x0], #16
    b       .L_bss_init_loop

    // Relocate the binary after initializing BSS
.L_relocate_binary:
    // x0 = src, the address the binary got loaded to
    ADR_REL x0, __binary_nonzero_start
    // x1 = dest, the address the binary was linked to
    ADR_ABS x1, __binary_nonzero_start
    // x2 = end
    ADR_ABS x2, __binary_nonzero_end_exclusive

    // Basically, we are doing:
    // memcpy(dest, src, end - start);
.L_copy_loop:
    // tmp = *src; src += 8;
    ldr     x3, [x0], #8
    // *dest = tmp; dest += 8;
    str     x3, [x1], #8
    // repeat until src >= end
    cmp     x1, x2
    b.lo    .L_copy_loop

    // Set the stack pointer
    ADR_ABS x0, __boot_core_stack_end_exclusive
    mov     sp, x0

    // Jump to the relocated Rust code
    ADR_ABS x1, _start_rust
    br      x1

.L_parking_loop:
    wfe
    b       .L_parking_loop

.size   _start, . - _start
.type   _start, function
.global _start

// vim: ft=asm
