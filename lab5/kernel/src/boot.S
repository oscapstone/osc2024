.section ".text.boot"

.global _start

_start:
    bl from_el2_to_el1

set_exception_vector_table:
    adr x1, exception_vector_table // vbar_el1: Vector Base Address Register (EL1)
    msr vbar_el1, x1               //           Holds the exception base address for any exception that is taken to EL1.

setup_stack:
    ldr     x1, =_stack_top
    mov     sp, x1

setup_bss:
    ldr     x1, =_bss_top
    ldr     w2, =_bss_size

init_bss:
    cbz     w2, run_main
    str     xzr, [x1], #8
    sub     w2, w2, #1
    cbnz    w2, init_bss

run_main:
    ldr     x1, =dtb_ptr
    str     x0, [x1], #8
    bl      main

proc_hang:
    wfe
    b       proc_hang

from_el2_to_el1:
    mov x1, (1 << 31)                  // EL1 uses aarch64
    msr hcr_el2, x1
    msr sctlr_el1, xzr
    mov x1, 0x3c5                      // EL1h (SPSel = 1) with interrupt disabled
    msr spsr_el2, x1
    msr elr_el2, lr
    eret                               // return to EL1