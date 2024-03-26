#include "mm.h"

.section ".text.boot"

.globl _start
_start:
    mrs x1, mpidr_el1
    and x1, x1, #3
    cbz x1, master
    b proc_hang

proc_hang:
    b proc_hang

master:
	mov x10, x0
    adr x0, bss_begin 
    adr x1, bss_end
    sub x1, x1, x0
    bl memzero
 
	mov x0, x10
    mov sp, #LOW_MEMORY
    bl kernel_main
