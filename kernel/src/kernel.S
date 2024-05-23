.section ".text._start_kernel"

.global _start

_start:
    // set stack before our code
    ldr     x1, =_start
    mov     sp, x1

    // clear bss
    ldr     x1, =__bss_start
    ldr     w2, =__bss_size
3:  cbz     w2, _start_kernel
    str     xzr, [x1], #8
    sub     w2, w2, #1
    cbnz    w2, 3b

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
