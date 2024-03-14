.section ".text.boot"

.global _start

_start:
    mrs     x1, mpidr_el1
    and     x1, x1, #3
    cbz     x1, setup

wait:  
    b   wait
setup:
    ldr    x1, =_start
    mov    sp, x1
    ldr    x1, =__bss_start // address var
    ldr    w2, =__bss_size // value var
set0:  
    cbz    w2, start_run
    str    xzr, [x1], #8 // clear 64-bit word memory by the storing value zero
    sub    w2, w2, #1
    cbnz   w2, set0
start_run:
    bl    main
    b     wait