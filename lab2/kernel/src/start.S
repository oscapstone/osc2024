.section ".text.boot"

.global _start

_start:
	// read cpu id and stop slave cores
	mrs x1, mpidr_el1
	and x1, x1 ,#3
	cbz x1, set_sp

halt:
	wfe
	b halt

set_sp:
	// set stack pointer to the _start
	ldr x1, =_start
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
	bl main
	b halt

