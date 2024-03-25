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

    adr     x0, __bss_start
    adr     x1, __bss_end
    bl      memzero

    // restore dtb addr
    mov     x0, x19
    bl      kernel_main

    b       prog_hang
