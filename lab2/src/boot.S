#include "mm.h"

.section ".text.boot"

.globl _start

_start:
	mrs	x0, mpidr_el1		
	and	x0, x0,#0xFF		// Check processor id
	cbz	x0, master		// Hang for all non-primary CPU
	b	proc_hang

proc_hang: 
	b 	proc_hang

master:
	adr	x0, bss_begin
	adr	x1, bss_end
	sub	x1, x1, x0
	bl 	memzero

	ldr x1, =_start
    mov sp, x1
	bl	main
	b 	proc_hang		// should never come here

.section .data
.global dtb_base
dtb_base:
.dc 0x0