.section ".text.boot"

.global _start

_start:
    mrs x0, mpidr_el1
    and x0, x0, #0xff
    cbz x0, master
    b proc_hang

master:
    ldr     x1, =_start
    mov     sp, x1
    ldr     x1, =__bss_start
    ldr     w2, =__bss_size
clear_bss:
    cbz     w2, run_main
    str     xzr, [x1], #8
    sub     w2, w2, #1         // set the value of w2 as zero
    cbnz    w2, clear_bss

run_main:
    bl      main
proc_hang:
    wfe
    b       proc_hang
