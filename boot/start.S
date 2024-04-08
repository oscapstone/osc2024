.section .text.start
.global _start

_start:
    // move device tree's addr
    mov     x28, x0

    // set stack pointer
    ldr     x1, =__stack_top
    mov     sp, x1

pre_relocate:
    // set prerequisite
    ldr     x1, =__prog_size
    ldr     x2, =__original_start
    ldr     x3, =__relocate_start

    //__relocate_start + (main - __original_start)
    ldr     x7, =main
    sub     x7, x7, x2
    add     x7, x7, x3

    cbz     x1, c_entry       // __prog_size 都搬完就跳到下個 function
relocate:
    ldr     x5, [x2], #8
    str     x5, [x3], #8
    sub     x1, x1, #8
    cbnz    x1, relocate

c_entry:
    // jump to relocated C code
    //ldr     x1, =__relocate_main
    br      x7
    //bl      main
