#include "mm.h"

.section ".text.boot"

.globl _start
_start:
	mov x25, x0 // move dtb loading address away
	mrs	x0, mpidr_el1
	and x0, x0, #0xFF
	cbz	x0, bss_ini
	b proc_hang

proc_hang:
	wfe
	b proc_hang

bss_ini:
	adr	x0, bss_begin
	adr	x1, bss_end
	sub	x1, x1, x0
	bl	memzero

relocation:
	ldr x2, =Bootloader_main
	ldr x3, =__bootloader_head
	ldr x4, =__bootloader_end
	sub x4, x4, x3 // bootloader size
	sub x2, x2, x3 // Bootloader_main's distance from __bootloader_head
	ldr x5, =__ini_pos
	ldr x6, =__new_pos

move_bootloader:
	ldr x7, [x5], #8
	str x7, [x6], #8
	sub x4, x4, #8
	cbnz x4, move_bootloader

exec:
	ldr x7, =__stack_top
	mov sp, x7
	ldr x6, =__new_pos
	add x6, x6, x2
	br x6 // branch to new-pos Bootloader_main
