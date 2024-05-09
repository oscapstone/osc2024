.section ".text.boot"

.global _start

_start:
    mrs     x1, mpidr_el1
    and     x1, x1, #3
    cbz     x1, master_core

proc_hang:
    wfe
    b       proc_hang

master_core:
    ldr     x1, =_stack_top
    mov     sp, x1
    ldr     x1, =_bss_top
    ldr     w2, =_bss_size

clear_bss:
    cbz     w2, run_main
    str     xzr, [x1], #8
    sub     w2, w2, #1
    cbnz    w2, clear_bss

run_main:
    bl      main
    b       proc_hang
