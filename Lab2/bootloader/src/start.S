.section ".text.boot"

.global _start

_start:
    mov     x10, x0 // dtb address
    mov     x11, x1 /* x1-x3 must is 0 */
    mov     x12, x2
    mov     x13, x3

    // relocate our code from load address to link address
1:  ldr     x1, =0x80000
    ldr     x2, =_start
    ldr     w3, =_loader_size
2:  ldr     x4, [x1], #8
    str     x4, [x2], #8
    sub     w3, w3, #1
    cbnz    w3, 2b

    // set stack before our code
    // ldr loads data from memory to register, which is _start here
    ldr     x1, =_start
    mov     sp, x1

    // clear bss
    ldr     x1, =_bss_start 
    ldr     w2, =_bss_size
3:  cbz     w2, 4f
    # set the content x1 points to as zero
    # and x1 = x1 + 8
    str     xzr, [x1], #8
    # w2 = w2 - 1
    sub     w2, w2, #1
    cbnz    w2, 3b

    // jump to C code, should not return

4:  bl      main - 0x20000
// 4:  bl      main
    // for failsafe, halt this core too
    b       1b