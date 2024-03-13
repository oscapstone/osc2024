.section ".text.boot"

.global _start      /* symbol '.global' will make 'start' globally visible */

_start:             /* '_start' tag point to the begining of the program */
    ldr     x1, =_start     /* load register */
    mov     sp, x1
    ldr     x1, =__bss_start
    ldr     w2, =__bss_size

clear_bss:
    cbz     w2, run_main    /* compare and branch if zero */
    str     xzr, [x1], #8   /* str: store, xzr: zero register, #: immediate value */
    sub     w2, w2, #1
    cbnz    w2, clear_bss   /* end */

run_main:
    bl      main            /* branch with link */

proc_hang:
    wfe                     /* wait for event */
    b       proc_hang       /* branch to proc_hang */