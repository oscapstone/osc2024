.section ".text.boot"

.global _start

_start:
	// move bootloader from 0x80000 to 0x60000
	ldr x1, =0x80000
	ldr x2, =__bootloader_start
	ldr x3, =__bootloader_size
_relocate:
	ldr x4, [x1], #8
	str x4, [x2], #8
	sub x3, x3, #1
	cbnz x3, _relocate

set_sp:
	// set stack pointer to the _start
	ldr x1, =__bootloader_start
	mov sp, x1

	// clear bss
	ldr x1, =__bss_start
	ldr x2, =__bss_size

clear_bss:
	cbz x2, jump_to_main
	str xzr, [x1], #8
	sub x2, x2, #1
	cbnz x2, clear_bss

jump_to_main:
	bl main-0x20000

halt:
	wfe
	b halt

