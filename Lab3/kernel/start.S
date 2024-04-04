.section ".text.boot"

.global _start

_start:
    // read cpu id, stop slave cores
    mrs     x1, mpidr_el1
    and     x1, x1, #3
    cbz     x1, bss_setting
    bl      from_el2_to_el1
    mrs     x1, CurrentEL
    ubfx    x1, x1, #2, #2
    cmp     x1, #1
    bne     proc_hang

    # bl      set_exception_vector_table

    // cpu id > 0, stop
proc_hang:  
    wfe
    b       proc_hang

bss_setting:  // cpu id == 0
    // set top of stack just before our code (stack grows to a lower address per AAPCS64)
    ldr     x1, =_start
    mov     sp, x1

    // clear bss
    ldr     x1, =__bss_start
    ldr     w2, =__bss_size
    
bss_init:  
    cbz     w2, 4f
    str     xzr, [x1], #8
    sub     w2, w2, #1
    cbnz    w2, bss_init

    // jump to C code, should not return
4:  bl      main
    // for failsafe, halt this core too
    b       proc_hang