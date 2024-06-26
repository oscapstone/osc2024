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

	bl from_el2_to_el1  // change el2 -> el1

    // set exception vector table
    adr x0, exception_vector_table
    msr vbar_el1, x0

	// move value of x10~x13 to x0~x3	
	mov     x0, x10
    mov     x1, x11
    mov     x2, x12
    mov     x3, x13
	
    ldr x4, =_start
	mov	sp, x4
	bl	kernel_main
	b 	proc_hang		// should never come here

from_el2_to_el1:
    mov x0, (1 << 31) // EL1 uses aarch64
    msr hcr_el2, x0
    mov x0, 0x3c5 // EL1h (SPSel = 1) with interrupt disabled
    msr spsr_el2, x0
    msr elr_el2, lr
    eret // return to EL1

