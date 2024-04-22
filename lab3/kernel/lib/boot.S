.section ".text"

.global _start

_start:
  // read cpu id, stop slave cores
  // system register need use mrs
    mrs     x1, mpidr_el1
    and     x1, x1, #3
    // switch to el1 before master
    cbz     x1, master
  // cpu id > 0, stop

busyloop:  
    wfe
    b       busyloop

from_el2_to_el1:
    // enable RW
    mov x20, (1 << 31) // EL1 uses aarch64
    msr hcr_el2, x20    
    // 3c daif disabled
    // 5 exception level
    mov x20, 0x3c5 // EL1h (SPSel = 1) with interrupt disabled
    msr spsr_el2, x20
    msr elr_el2, lr     // eret location
    eret // return to EL1 ,str in elr_el2

master:
    mov x21, x0
    bl set_exception_vector_table
    bl from_el2_to_el1

    ldr     x1, =_start
    mov     sp, x1

    ldr     x1, =__bss_start
    ldr     w2, =__bss_size

bssinitloop:  
    cbz     w2, kernel
    // save 0 and plus 8
    str     xzr, [x1], #8
    sub     w2, w2, #1
    cbnz    w2, bssinitloop

kernel:
    // bl save next command to lr register
    bl      main
    b       busyloop