.section ".text.boot"

.global _start

// x0 register stores address to dtb
_start:
    // clear bss
    ldr     x20, =_bss_start
    ldr     x21, =_bss_size
    add     x21, x21, x20
3:  
    cmp     x20, x21
    b.eq    4f
    str     xzr, [x20]
    add     x20, x20, #8
    b       3b

4:  
    // initialize sp
    ldr     x20, =_start
    mov     sp, x20

    // jump to C code, should not return
    bl      kernel_main
5:
    // for failsafe, halt this core too
    b       5b