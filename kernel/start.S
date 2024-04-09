.section .text

_start:
    // move devicetree's addr (ENABLE WHEN USING QEMU)

    // switch from EL2 to EL1
    bl      from_el2_to_el1

    // set stack pointer
    ldr     x1, =__stack_top
    mov     sp, x1

    // clear bss segment
    ldr     x1, =__bss_start
    ldr     x2, =__bss_size
1:  cbz     x2, 2f
    str     xzr, [x1], #8
    sub     x2, x2, #1
    cbnz    x2, 1b

    // jump to C code
2:  bl      main

from_el2_to_el1:
    mov     x0, (1 << 31)
    msr hcr_el2, x0 // use AARCH64 (64-bit)
    mov x0, 0x3c5 // EL1h (SPSel = 1) with interrupt disabled
    msr spsr_el2, x0
    msr elr_el2, lr
    eret // return to EL1
