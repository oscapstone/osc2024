.section ".text.boot"

.global _start
.global prog_hang

_start:
	// move dbt ptr to an unused register
	mov x19, x0

	// read cpu id, stop slave cores
	// the last 8 bits indicate the core number in cortex-a53
	mrs x1, mpidr_el1
	and x1, x1, #3
	cbz x1, 1f

	// if core number > 0, let the core hangs

prog_hang:
	wfe
	b prog_hang

1:
	// set top of stack just before our code (stack grows to a lower address)
	ldr x1, =_start
	mov sp, x1

	ldr x1, =__bss_start

	// w- registers are the 32-bit version of x- register
	ldr w2, =__bss_size

2:
	// clear bss
	cbz  w2, 3f
	str  xzr, [x1], #8
	sub  w2, w2, #1
	cbnz w2, 2b

3:
	// load dbt ptr to the first parameter
	mov x0, x19

	// jump to C code, should not return
	bl main

	// for failsafe, halt this core too
	b prog_hang

