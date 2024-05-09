.section ".text.boot"

.global _start
.extern relocated_flag

_start:
    ldr     x1, =_dtb
    str     x0, [x1], #8
    ldr     x1, =relocated_flag
    ldr     w1, [x1]
    cmp     w1, #1
    beq     setup_stack
    ldr     x1, =_relocated_stack_top
    mov     sp, x1
    b       run_main

setup_stack:
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

proc_hang:
    wfe
    b       proc_hang
