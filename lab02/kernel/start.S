.section ".text.boot"

.global _start

_start:


    // read cpu id
    mrs x0, mpidr_el1
    
    // only use core0, the other core goes into infinite loop
    and x0, x0, #3
    cbz x0, 2f

1:  
    wfe
    b       1b
2:  // cpu id == 0

    // set top of stack just before our code (stack grows to a lower address per AAPCS64)
    ldr     x0, =_start
    mov     sp, x0

    // clear bss
    ldr     x0, =__bss_start
    ldr     w2, =__bss_size
3:  
    cbz     w2, 4f
    str     xzr, [x0], #8
    sub     w2, w2, #1
    cbnz    w2, 3b

    // jump to C code, should not return
4:  

    bl      main
    // for failsafe, halt this core too
    b       1b