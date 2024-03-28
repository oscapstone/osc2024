#include "mm.h"

.section ".text.boot"

.globl _start

_start:
	mov x10, x0
	mov x11, x1
	mov x12, x2
	mov x13, x3

	mrs	x0, mpidr_el1		
	and	x0, x0,#0xFF		// Check processor id
	cbz	x0, master		// Hang for all non-primary CPU
	b	proc_hang

proc_hang: 
	wfe
	b 	proc_hang

master:
	ldr x1, =0x80000
	ldr x2, =__bootloader_start
	ldr x3, =__bootloader_size

relocate:
	ldr x4, [x1], #8
	str x4, [x2], #8
	sub x3, x3, #1
	cbnz x3, relocate

	ldr x1, =_start
	mov sp, x1

	ldr	x0, =bss_begin
	ldr	w2, =__bss_size

bss_reset:
    cbz w2, run_main
    str xzr, [x1], #8
    sub w2, w2, #1
    cbnz w2, bss_reset

run_main:
	bl main-0x20000
	b run_main