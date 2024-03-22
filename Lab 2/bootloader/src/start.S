    .section ".text.boot"
    .global _start
_start:
    adr     x10, .                      // load address (0x80000)
    ldr     x11, = _start               // link address (0x60000)
    ldr     x12, =_bootloader_size

    // set stack pointer
    mov     sp, x11

reloacate:
    ldr     x13, [x10], #8              // x4 = [x1]; x1 += 8;
    str     x13, [x11], #8              // [x2] = x4; x2 += 8;
    sub     w12, w12, #1
    cbnz    w12, reloacate

    // clear bss
    ldr    x10, =_bss_begin
    ldr    x11, =_bss_size
    bl     memzero

    // jump to c, should not return
    bl      main - 0x20000

hang:
    // Hang for all non-primary CPU and this core when c code failed
    wfe
    b       hang

memzero:
    str     xzr, [x10], #8
    subs    x11, x11, #8
    b.gt    memzero
    ret