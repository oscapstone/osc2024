.section ".text.boot"

.global _start

_start:
    /* save dtb address */
    mov x5, x0    

    /* relocation */
    ldr x1, =0x80000
    ldr x2, =_start
    ldr w3, =boot_size

relocation:
    ldr x4, [x1], #8
    str x4, [x2], #8
    sub w3, w3, #1
    cbnz w3, relocation

    /* set stack pointer */
    ldr x1, =_start
    mov     sp, x1
    
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
    bl main-0x20000

