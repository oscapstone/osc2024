.section ".text.boot"

// functions
.global _start

_start:
	

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
	// load the dtb addr
	mov		x1, 0x70000
	ldr		x2, [x1]
	ldr		x3, =_dtb_ptr
	str		x2, [x3]
	
	bl		main					// branch label: will link in linking state
	b		halt

// variables
.global _dtb_ptr

.section ".data"
_dtb_ptr: .quad 0x0