.section ".text.boot"

.global _start

_start:
    // read cpu id, stop slave cores
    mrs     x0, mpidr_el1
    and     x0, x0, #3  //Check processor id
    cbz     x0, 2f      //if CPU id = 0, jump to 2f
    b       1f

1:  wfe
    b       1b

2:  ldr     x0, =_start
    mov     sp, x0
    ldr     x0, =__bss_start
    ldr     w1, =__bss_size

3:  cbz     w1, 4f
    str     xzr, [x0], #8  // x0* = xzr, x0 += 8 (write xzr represents discard, read xzr == 0)
    sub     w1, w1, #1
    cbnz    w1, 3b

4:  bl      main
    b       1b