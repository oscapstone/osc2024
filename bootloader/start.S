.section ".text.boot"

.global _start

_start:
	// read cpu id, stop slave cores
	// the last 8 bits indicate the core number in cortex-a53
	mrs x1, mpidr_el1
	and x1, x1, #3
	cbz x1, set_loader

	// if core number > 0, let the core hangs
halt:
	wfe
	b halt

set_loader:
	ldr x1, =__bootloader_start
	ldr w2, =__bootloader_size
	ldr x3, =0x80000

relocate:
	ldr  x4, [x3], #8 // load x3 and x3 = x3 + 8
	str  x4, [x1], #8 // store x4 to 0x60000
	sub  w2, w2, #1
	cbnz w2, relocate

setup_sp_and_bss:
	// set top of stack to 0x60000
	ldr x1, =_start
	mov sp, x1

	ldr x1, =__bss_start
	ldr w2, =__bss_size

clear_bss:
	cbz  w2, jump_to_main
	str  xzr, [x1], #8
	sub  w2, w2, #1
	cbnz w2, clear_bss

jump_to_main:
	// jump to bootloader
	// bl will set x30, make it a subroutine call
	// because we are in 0x8XXXX, but text section is located at 0x6XXXX
	// we need to - 0x20000 to get correct relative address
	bl main - 0x20000

	// for failsafe, halt this core too
	b halt
