.section ".text"
_start:
  // read cpu id, stop slave cores
  // system register need mrs
    mrs     x1, mpidr_el1
    and     x1, x1, #3
    cbz     x1, master
  // cpu id > 0, stop

busyloop:  
    wfe
    b       busyloop

master:
    ldr     x1, =_start
    mov     sp, x1

    ldr     x1, =__bss_start
    ldr     w2, =__bss_size

  // clear bss
bssinitloop:  
    cbz     w2, kernel
    str     xzr, [x1], #8
    sub     w2, w2, #1
    cbnz    w2, bssinitloop

kernel:
    bl      main
    b       busyloop

