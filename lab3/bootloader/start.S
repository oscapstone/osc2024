.section ".text.boot"

.global _start

_start:
    /* save registers x0 x1 x2 x3 */
    mov x10, x0 /* dtb_base address */
    mov x11, x1
    mov x12, x2
    mov x12, x3

    /* relocate bootloader */
    ldr x1, =0x80000
    ldr x2, =__loader_start // 0x60000
    ldr w3, =__loader_size

relocate:
    ldr x4, [x1], #8
    str x4, [x2], #8
    sub w3, w3, #1
    cbnz w3, relocate

    /* set stack pointer */
    ldr x1, =_start
    mov sp, x1

    /* clear bss section */
    ldr x1, =__bss_start
    ldr w2, =__bss_size

bss_reset:
    cbz w2, run_main
    str xzr, [x1], #8
    sub w2, w2, #1
    cbnz w2, bss_reset

run_main:
    /* branch to main function */
    bl main-0x20000
    b run_main
