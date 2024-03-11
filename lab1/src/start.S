.section ".text.boot"

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
	str		xzr, [x1], #0
	sub		w2, w2, #1
	cbnz	w2, clear_bss

kernel_main:
	bl		main
	b		halt