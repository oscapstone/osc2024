.section .text

_start:
    // set stack pointer
    ldr     x1, =__stack_top
    mov     sp, x1

    // TODO: clear bss segment

    // jump to C code
    bl      main
