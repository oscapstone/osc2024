    .section ".text.boot"
    .global _start
_start:
    // read cpu id, hang slave cores
    mrs     x10, mpidr_el1
    and     x10, x10, #0xFF       // Check processor id in last 8 bits
    cbnz    x10, hang          

master:
    // set top of stack just before our code (stack grows to a lower address per AAPCS64)
    ldr     x11, = _start
    mov     sp, x11

    // clear bss
    ldr    x10, =_bss_begin
    ldr    x11, =_bss_size
    bl     memzero

    // jump to c, should not return
    bl      main

    // Hang for all non-primary CPU and this core when c code failed
hang:
    wfe
    b       hang

memzero:
    str     xzr, [x10], #8
    subs    x11, x11, #8
    b.gt    memzero
    ret


/* 
     .global align
align:
    sub x1, x1, #1
    add x0, x0, x1
    mvn x1, x1
    and x0, x0, x1
    ret
*/