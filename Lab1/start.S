.section ".text.boot"

.global _start

_start:
    /* get cpu id */
    //mrs x1, mpidr_el1
    //and x1, x1, #3
    //cbnz x1, halt // halt if cpu id != 0

    /* set stack pointer */
    ldr x1, =_start
    mov sp, x1

    /* clear bss section */
    ldr x1, =bss_begin
    ldr w2, =bss_size

bss_init:
    cbz w3, run_main
    str xzr, [x1], #8
    sub w2, w2, #1
    cbnz w2, bss_init

run_main:
    /* branch to main function */
    bl main

halt:
    wfe
    b halt