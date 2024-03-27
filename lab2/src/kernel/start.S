.section ".text.boot"

.global _start


_start:
	// read the dtb data address from teacher's bootloader store in x0
	ldr		x1, =_dtb_ptr
	str		x0, [x1]
	
	// read cpu id, stop slave cores
	mrs     x1, mpidr_el1
	and		x1, x1, #3
	cbz		x1, init		// branch to init if x1 == 0
	
	// cpu id > 0, stop
halt:
	wfe
	b		halt

init:
	
	// init stack pointer, because the code start from _start so lower address space can be stack
	ldr		x1, =_start
	mov		sp, x1
	
	// init bss
	ldr		x1, =__bss_start
	ldr		w2, =__bss_size
clear_bss:
	cbz		w2, kernel_main
	str		xzr, [x1], #8			// write zero to [x1] and write x1 = [x1 + 8], xzr is 64 bit (8 bytes) zero register
	sub		w2, w2, #1				// w2 = w2 - 1
	cbnz	w2, clear_bss			// if (w2 != 0) jmp clear_bss

kernel_main:
	bl		main					// branch label: will link in linking state
	b		halt

.global _dtb_ptr
.section .data
_dtb_ptr: .long 0x0