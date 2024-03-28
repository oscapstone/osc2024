.section ".text.boot"

.global _start

_start:
    # Copy x0 dtb address to 0x60000
    mov x1, 0x50000
    str x0, [x1]

    # Copy code to addres 0x60000
    ldr x0, =0x80000
    ldr x1, =0x60000
    ldr x2, =__cpy_size

.L_copy_loop:
    ldr w3, [x0], #4
    str w3, [x1], #4
    subs x2, x2, #4
    bne .L_copy_loop

    ldr x0, =_boot
    br x0

_boot:
    // read cpu id, stop slave cores
    mrs     x1, mpidr_el1
    and     x1, x1, #3
    cbz     x1, 2f
    // cpu id > 0, stop
1:  wfe
    b       1b
2:  // cpu id == 0

    // set stack before our code
    ldr     x1, =_start
    mov     sp, x1

    // clear bss
    ldr     x1, =__bss_start
    ldr     w2, =__bss_size
3:  cbz     w2, 4f
    str     xzr, [x1], #8
    sub     w2, w2, #1
    cbnz    w2, 3b

    // jump to main function and never return
4:  bl      main
    // for failsafe, halt this core too
    b       1b

.size _start, . - _start
.type _start, %function
