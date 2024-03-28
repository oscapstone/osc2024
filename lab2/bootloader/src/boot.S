#include "mm.h"

.section ".text.relocate"
.global _relocate_asm
_relocate_asm:
    mov    x5, x0
    ldr    x1, =0x5FFF8
    str    x0, [x1]
    mrs    x0, mpidr_el1
    and    x0, x0, #0xFF
    cbz    x0, _initialize
    b      _proc_hang
_proc_hang:
    wfe
    b _proc_hang
_initialize:
    adr    x0, bss_begin
    adr    x1, bss_end
    sub    x1, x1, x0
    bl     memzero
    mov    sp, #LOW_MEMORY
_relocate_start:
    ldr    x0, =_begin
    ldr    x1, =_end
    ldr    x2, =_bootloader
    ldr    x3, =0
    sub    x1, x1, x0
_relocate_loop:
    ldr    x4, [x0]
    str    x3, [x0], #8
    str    x4, [x2], #8
    sub    x1, x1, #8
    cbnz   x1, _relocate_loop
_goto_newloc:
    ldr    x0, =_bootloader
    br     x0

.section ".text.boot"
.global _start
start:
    mov    x0, x5
    bl     kernel_main
    wfe
