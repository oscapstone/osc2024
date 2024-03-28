.section ".text.boot"

.global _start

_start:
    /* set stack pointer */
    ldr x1, =_start
    mov sp, x1

    /* clear bss section */
    ldr x1, =bss_begin
    ldr w2, =bss_size

bss_init:
    cbz w2, run_main
    str xzr, [x1], #8
    sub w2, w2, #1
    cbnz w2, bss_init

run_main:
    /* branch to main function */
    bl main

halt:
    wfe
    b halt