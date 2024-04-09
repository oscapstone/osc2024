#include "mm.h"

.section ".text.boot"

.globl _start

_start:
	mrs	x1, mpidr_el1		
	and	x1, x1,#0xFF		// Check processor id
	cbz	x1, master		// Hang for all non-primary CPU

proc_hang:
	wfe
	b 	proc_hang

master:
	ldr x1, =_start
    mov sp, x1
	ldr     x1, =bss_begin
    ldr     w2, =__bss_size

bss_reset:  
    cbz     w2, run_main
    // save 0 and plus 8
    str     xzr, [x1], #8
    sub     w2, w2, #1
    cbnz    w2, bss_reset

run_main:
	bl	main
	b 	proc_hang		// should never come here

