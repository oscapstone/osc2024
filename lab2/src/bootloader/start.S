.section ".text.boot"

.global _start

// boot loader start section, start from addr: 0x60000 but load in 0x80000 (config in linker.ld)
_start:

	// read cpu id, stop slave cores
	//mrs     x1, mpidr_el1
	//and		x1, x1, #3
	//cbz		x1, init		// branch to init if x1 == 0

//init:
	// storing dtb memory address from teacher's bootloader at the very begining
	mov		x1, 0x70000
	str		x0, [x1]

	mov		x1, 0x80000					// where to copy
	mov		x2, 0x60000					// where to paste
	ldr		x3, =__bootloader_size
	//adr		x1, main				// where to copy
	//mov		x2, 0x60000				// where to paste
	//ldr		x3, =__bootloader_end	// the end of the bootloader
	//add		x3, x3, #0x20000		// because the offset in linker offset by 0x20000
	//sub		x3, x3, x1				// x3 = __bootloader_end - main
	//mov		x3, x3, lsr #3			// x3 /= 8 (each move 8 bytes)
	
relocate:
	ldr		x4, [x1], #8	// x4 = MEM[x1], x1 += 8
	str		x4, [x2], #8	// MEM[x2] = x4, x2 += 8
	sub		x3, x3, #1		// w3--;
	cbnz	x3, relocate	// if (w3 != 0) goto relocate;

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
	bl		main - 0x20000
	//mov		x1, main - 0x20000
	//br		x1
	b		halt

halt:
	wfe
	b		halt
