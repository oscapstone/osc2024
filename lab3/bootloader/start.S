.section ".text.boot"

.global _start


_start:

    // disable other cpu, run the code just one time
    mrs     x1, mpidr_el1
    and     x1, x1, #3
    cbz     x1, init
    b       halt

init:
    // storing dtb memory address from teacher's bootloader at the very begining
    mov     x1, 0x70000
    str     x0, [x1]

    // copy the bootloader to 0x60000
    mov     x1, 0x80000
    mov     x2, 0x60000
    ldr     x3, =__bootloader_size__
relocate:
    ldr     x4, [x1], #8            // x4 = MEM[x1], x1 += 8
    str     x4, [x2], #8            // MEM[x2] = x4, x2 += 8
    sub     x3, x3, #1
    cbnz    x3, relocate

    // initialize the stack pointer, after moving the code
    // the code start at 0x60000
    mov     x1, 0x60000
    mov     sp, x1

    // clearing the bss section 
    ldr     x1, =__bss_start__
    ldr     x2, =__bss_size__
clear_bss:
    cbz     x2, bootloader_main
    str     xzr, [x1], #8
    sub     x2, x2, #1
    cbnz    x2, clear_bss

bootloader_main:
    bl      main - 0x20000
    b       halt

halt:
    wfe
    b      halt



