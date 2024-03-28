.section ".text.boot"

.global _start

_start:
    // read cpu id, to suspend other CPU
    mrs     x1, mpidr_el1
    and     x1, x1, #3
    cbz     x1, 2f

    // cpu id > 0, stop. to make only one CPU work.
1:  wfe
    b       1b
    // cpu id == 0
2:
    // set top of stack just before our code (stack grows to a lower address per AAPCS64)
    ldr     x1, = __stack_top
    mov     sp, x1

    // clear bss
    ldr     x1, =__bss_start
    ldr     w2, =__bss_size
3:  cbz     w2, 4f
    str     xzr, [x1], #8
    sub     w2, w2, #1
    cbnz    w2, 3b

4:  
    ldr     x1, = _dtb
    bl      main
    b       1b
