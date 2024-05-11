.section ".text.boot"

.global dtb_ptr

.global _start
.global halt

_start:
	bl from_el2_to_el1

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
	// put address of dtb into dtb_ptr
	ldr x1, =dtb_ptr
	str x0, [x1]

set_exception_vector_table:
  	adr x1, exception_vector_table
  	msr vbar_el1, x1

	bl main
	b halt

from_el2_to_el1:
    mov x1, (1 << 31) // EL1 uses aarch64
    msr hcr_el2, x1
    mov x1, 0x3c5 // EL1h (SPSel = 1) with interrupt disabled
    msr spsr_el2, x1
    msr elr_el2, lr
	ldr x1, =_start
	msr sp_el1, x1 // set sp_ep1
    eret // return to EL1