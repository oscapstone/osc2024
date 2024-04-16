.section ".text.boot"

.global _start

_start:
    // read cpu id, stop slave cores
    mrs     x1, mpidr_el1
    and     x1, x1, #3
    mov     x20, x0
    cbz     x1, relocation
    // cpu id > 0, stop
proc_hang:  
    wfe
    b       proc_hang

relocation:
    ldr    x0, =__begin         // load address
    ldr    x1, =__end           // load address
    sub    x1, x1, x0           // size of all sections
    ldr    x2, =__relocation    // relocation address

relocation_loop:
    ldr    x3, [x0], #8
    str    x3, [x2], #8
    sub    x1, x1,   #8
    cbnz   x1, relocation_loop

stack_setting:  
    // cpu id == 0

    // set top of stack just before our code (stack grows to a lower address per AAPCS64)
    ldr     x1, =_start
    mov     sp, x1

    // clear bss
    ldr     x1, =__bss_start
    ldr     w2, =__bss_size

bss_setting:  
    cbz     w2, prog
    str     xzr, [x1], #8
    sub     w2, w2, #1
    cbnz    w2, bss_setting

    // jump to C code, should not return
prog:  
    ldr     x0, =main
    ldr     x1, =__begin
    ldr     x2, =__relocation

    sub     x0, x0, x1
    add     x0, x0, x2
    
    br      x0
    // for failsafe, halt this core too
    b       proc_hang