.section ".text.boot"

.global _start

_start:
    /* get cpu id */
    mrs x1, mpidr_el1
    and x1, x1, #3
    cbz x1, 2f
1:
    /* halt if cpu id != 0 */
    wfe
    b 1b
2:
    /* set stack pointer */
    ldr x1, =_start
    mov sp, x1

    /* clear bss section */
    ldr x1, =__bss_start
    ldr w2, =__bss_size
3:
    cbz w3, 4f
    str xzr, [x1], #8
    sub w2, w2, #1
    cbnz w2, 3b
4:
    /* branch to main function */
    bl main
    b 1b
