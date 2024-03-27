.section .text

_start:
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
