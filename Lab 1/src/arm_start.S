.section ".text.boot"

.global _start

_start:
    // read cpu id, hang slave cores
    mrs     x0, mpidr_el1
    and     x0, x0, #0xFF       // Check processor id in last 8 bits
    cbz     x0, master          
    
hang:                           // Hang for all non-primary CPU
    wfe
    b       hang

memzero:
    str     xzr, [x0], #8
    subs    x1, x1, #8
    b.gt    memzero
    ret

master:                         // cpu id == 0
    adr    x0, bss_begin
    adr    x1, bss_end
    sub    x1, x1, x0
    bl     memzero

    // set top of stack just before our code (stack grows to a lower address per AAPCS64)
    ldr     x1, = _start
    mov     sp, x1

    bl      kernel_main         // jump to C code, should not return
    b       hang                // for failsafe, halt this core too


    