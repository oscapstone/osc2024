.section ".text.boot"

.global _start

_start:
    /* relocate bootloader */
    ldr x1, =0x80000             //address for relocation.
    ldr x2, =__bootloader_start
    ldr w3, =__bootloader_size

relocate:
    ldr x4, [x1], #8
    str x4, [x2], #8
    sub w3, w3, #1
    cbnz w3, relocate

    /* set stack pointer */
    ldr x1, =_start
    mov sp, x1


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
    bl      main-0x20000
    // for failsafe, halt this core too
    b       halt

halt:  
    b       halt 
