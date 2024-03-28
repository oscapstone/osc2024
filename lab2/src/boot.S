#include "../include/mm.h"

.section ".text.boot"

.globl _start
_start:
	//  Stroe the value of x0
	mov     x10, x0
    mov     x11, x1
    mov     x12, x2
    mov     x13, x3
	
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

	// move value of x10~x13 to x0~x3	
	mov     x0, x10
    mov     x1, x11
    mov     x2, x12
    mov     x3, x13
	
	mov	sp, #LOW_MEMORY
	bl	kernel_main
	b 	proc_hang		// should never come here