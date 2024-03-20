.section ".text.boot"

.global _start

_start:
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
    ldr     x1, =_dtb // let x1 point to the space allocated to '_dtb'
    str     x0, [x1], #8 /* x0 usually store importent arg to pass it into kernel, here, DTB */
    bl      main

proc_hang:
    wfe
    b       proc_hang