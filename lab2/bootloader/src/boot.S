#include "mm.h"

.section ".text.boot"

.globl _start
_start:
    mov x19, x0

	mrs	x0, mpidr_el1		
	and	x0, x0,#0xFF	// Check processor id
	cbz	x0, master		// Hang for all non-primary CPU
	b	proc_hang

proc_hang: 
	b 	proc_hang

master:
	ldr     x1, 	=_kernel
    ldr     x2, 	=_bootloader
    ldr     w3, 	=__loader_size
relocate:
  	ldr     x4, 	[x1], 	#8
    str     x4, 	[x2], 	#8
    sub     w3, 	w3, 	#1
    cbnz    w3, 	relocate

	adr		x0, 	bss_begin
	adr		x1, 	bss_end
	sub		x1, 	x1, 	x0
	bl 		memzero

	// ldr     x1, 	=_kernel
	// sub		x1, 	x1,		0x4
	mov 	sp, 	#0x70000

    
    mov     x0,     x19
	bl 		kernel_begin - 0x20000


