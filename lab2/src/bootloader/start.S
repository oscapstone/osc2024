.section ".text.boot"

.global _start

// boot loader start section, start from addr: 0x60000 but load in 0x80000 (config in linker.ld)
_start:

	// read cpu id, stop slave cores
	mrs     x1, mpidr_el1
	and		x1, x1, #3
	cbz		x1, init		// branch to init if x1 == 0

init:
	// storing dtb memory address from teacher's bootloader at the very begining
	ldr		x1, =0x0
	str		x0, [x1]

	ldr		x1, =0x80000				// where to copy
	ldr		x2, =__bootloader_start		// where to paste
	ldr		x3, =__bootloader_size

relocate:
	ldr		x4, [x1], #8	// x4 = MEM[x1]
	str		x4, [x2], #8	// MEM[x2] = x4
	sub		x3, x3, #1
	cbnz	x3, relocate

	// init stack pointer, because the code start from _start so lower address space can be stack
	ldr		x1, =_start
	mov		sp, x1
	
	// init bss
	ldr		x1, =__bss_start
	ldr		w2, =__bss_size
clear_bss:
	cbz		w2, bootloader_main
	str		xzr, [x1], #8			// write zero to [x1] and write x1 = [x1 + 8], xzr is 64 bit (8 bytes) zero register
	sub		w2, w2, #1				// w2 = w2 - 1
	cbnz	w2, clear_bss			// if (w2 != 0) jmp clear_bss

bootloader_main:
	//ldr		x1, 0x60000
	//ldr		pc, x1
	bl		main - 0x20000
	b		halt

halt:
	wfe
	b		halt

bootloader_addr: .long 0x60000