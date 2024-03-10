.section ".text.boot"

.global _start


_start:
	
	mrs     x1, mpidr_el1
	and		x1, x1, #3
	cbz		x1, init		// branch to init if x1 == 0

halt:
	wfe
	b		halt

init:
	
	// init stack pointer
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
	b		main
	b		halt