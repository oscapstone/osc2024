.section ".text._start"

.global _start
_start:
    // save dtb addr
    mov     x19, x0

    mrs     x0, MPIDR_EL1
    and     x0, x0, #0x3
    cbz     x0, primary_cpu

.global prog_hang
prog_hang:
    wfe
    b       prog_hang

primary_cpu:
    adr     x0, __boot_core_stack_end
    mov     sp, x0

    adr     x0, __bootloader
    ldr     x1, =__bootloader
    cmp     x0, x1
    b.gt    relocate

    adr     x0, __bss_start
    adr     x1, __bss_end
    bl      memzero

    // restore dtb addr
    mov     x0, x19
    bl      bootloader_main

    b       prog_hang

relocate:
    adr     x1, __bootloader
    adr     x2, __data_end
    sub     x2, x2, x1
    ldr     x0, =__bootloader
    bl      memcpy

    mov     x1, x0
    // restore dtb addr
    mov     x0, x19
    br      x1

    b       prog_hang
