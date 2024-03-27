.section ".text.boot"

.global _start

_start:
    // read cpu id, stop slave cores
    mrs     x1, mpidr_el1 // get cpu id
    and     x1, x1, #3 // mask off cpu id bits (cpu id is in bits 0-1)
    cbz     x1, set_stack  // cpu id == 0, continue
halt:  
    // cpu id > 0, loop
    wfe //(Wait for event status)
    b       halt // loop
set_stack:  
    // cpu id == 0
    // set top of stack just before our code (stack grows to a lower address per AAPCS64)
    ldr     x1, =_start // get address of this code
    mov     sp, x1  // set stack pointer
    // clear bss
    ldr     x1, =__bss_start // get start of bss
    ldr     w2, =__bss_size // get size of bss
bss_loop:  
    // clear bss loop
    cbz     w2, run_main
    str     xzr, [x1], #8 // set 8 bytes to 0
    sub     w2, w2, #1
    cbnz    w2, bss_loop // loop
run_main:  
    // jump to C code, should not return
    bl      main
    // for failsafe, halt this core too
    b       halt