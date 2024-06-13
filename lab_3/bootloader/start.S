.section ".text.relocate"
.global _relocate

_relocate:
    // relocate our code from load address to link address
    ldr     x1, =0x80000;  // src address 
    ldr     x2, =0x60000;  // target address
    ldr     w3, =__loader_size
1:  ldr     x4, [x1], #8
    str     x4, [x2], #8
    sub     w3, w3, #1
    cbnz    w3, 1b

    ldr     x1, =_start
    mov     sp, x1

    // clear bss
    ldr     x1, =__bss_start
    ldr     w2, =__bss_size
2:  cbz     w2, 3f
    str     xzr, [x1], #8
    sub     w2, w2, #1
    cbnz    w2, 2b


3:  // Device tree address has been moved from x0 to x10 within the bootloader.
    mov x0, x10
    bl      relocator-0x20000
    b _start


.section ".text.boot"

.global _start

_start:
    // jump to main function in C
    bl      0x80000
    // halt this core if return
1:
    wfe
    b       1b

