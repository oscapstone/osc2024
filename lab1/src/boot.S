#include "mm.h"

.section ".text.boot"

.globl _start
_start:
	mrs	x0, mpidr_el1 // mrs: move system reg to general-purpose reg, 
	and x0, x0, #0xFF // If the current process ID is 0, then execution is transferred to the master function
	cbz	x0, master // cbz: compare and branch on zero
	b proc_hang

proc_hang:
	b proc_hang

master: // clean the .bss section by calling memzero
	adr	x0, bss_begin
	adr	x1, bss_end
	sub	x1, x1, x0
	bl	memzero // in ARMv8 architecture, by convention, 
	// the first seven arguments are passed to the called function via registers x0–x6.
	// args: start address(bss_begin), size(bss_end - bss_begin)

	mov	sp, #LOW_MEMORY
	bl	kernel_main
