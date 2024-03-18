    .section ".text.boot"
    .global _start

_start:

// reloacate this code from load address to link address
    adr     x1, .                   // load address (0x80000)
    ldr     x2, =_start             // link address (0x60000)
    ldr     w3, =_bootloader_size

memmove:
    ldr     x4, [x1], #8            // x4 = [x1]; x1 += 8;
    str     x4, [x2], #8            // [x2] = x4; x2 += 8;
    sub     w3, w3, #1
    cbnz    w3, memmove

master:                         // cpu id == 0
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
