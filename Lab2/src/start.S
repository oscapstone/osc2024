.section ".text.boot"

.global _start

_start:
    // read cpu id, stop slave cores
    mrs     x1, mpidr_el1
    and     x1, x1, #3
    cbz     x1, bss_init
    // cpu id > 0, stop

halt:  
    wfe
    b       halt 

// cpu id == 0
bss_init:  
    // clear bss
    ldr     x1, =__bss_start
    ldr     w2, =__bss_size

clear_bss_memory:  
    cbz     w2, jump_to_main
    str     xzr, [x1], #8
    sub     w2, w2, #1
    cbnz    w2, clear_bss_memory

// jump to C code, should not return
jump_to_main:  
    bl      main
    // for failsafe, halt this core too
    b       halt
