.section ".text.boot"

.global _start

_start:
    ldr x0, =0x80000
    ldr x1, =__pseudo_start
    ldr x2, =__pseudo_size

self_relocate:
    ldr x3, [x0], #8
    str x3, [x1], #8
    sub x2, x2, #1
    cbnz x2, self_relocate

set_sp:
    ldr x1, =_start
    mov sp, x1
    ldr x0, =__bss_start
    ldr x1, =__bss_size

clear_bss:
    cbz x1, bootloader_main
    str xzr, [x0], #8
    sub x1, x1, #1
    cbnz x1, clear_bss

pseudo_main:
    bl bootloader_main
    b pseudo_main