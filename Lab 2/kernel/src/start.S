    .section ".text.boot"
    .global _start

_start:

_start:
    // read cpu id, hang slave cores
    mrs     x0, mpidr_el1
    and     x0, x0, #0xFF       // Check processor id in last 8 bits
    cbnz    x0, hang          

master:
// set top of stack just before our code (stack grows to a lower address per AAPCS64)
    ldr     x1, = _start
    mov     sp, x1

// clear bss
    adr    x0, _bss_begin
    adr    x1, _bss_size
    bl     memzero

// jump to c, should not return
    bl      main

// Hang for all non-primary CPU and this core when c code failed
hang:
    wfe
    b       hang

memzero:
    str     xzr, [x0], #8
    subs    x1, x1, #8
    b.gt    memzero
    ret
