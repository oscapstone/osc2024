.section ".text.boot"

#include "arm/sysregs.h"

.global _start

// transform p to physical address in kernel mode (EL1)
#define PA(p) (p - 0xffff000000000000)

_start:

    // read cpu id, stop slave cores
    // mrs: read system register to the general register
	mrs     x1, mpidr_el1
	and		x1, x1, #0xff
	cbz		x1, init		// branch to init if x1 == 0
	
	// cpu id > 0, stop
    b       halt

init:
	// load the dtb addr
	mov		x1, 0x70000
	ldr		x2, [x1]
	ldr		x3, =PA(_dtb_ptr)
	str		x2, [x3]

	// init bss
	ldr		x1, =PA(__bss_start__)
	ldr		x3, =PA(__bss_end__)
	//ldr		x1, =__bss_start__
	//ldr		x3, =__bss_end__
	sub		x2, x3, x1
	lsr		x2, x2, #3
clear_bss:
	cbz		x2, kernel_main
	str		xzr, [x1], #8			// write zero to [x1] and write x1 = [x1 + 8], xzr is 64 bit (8 bytes) zero register
	sub		x2, x2, #1				// w2 = w2 - 1
	cbnz	x2, clear_bss			// if (w2 != 0) jmp clear_bss

    // TODO: cpacr_el1
    //ldr     x2, =CPACR_EL1_VAL
    //msr     cpacr_el1, x2

	// to exception level 1
	bl		from_el2_to_el1

el1_entry:
	
	// init stack pointer, because the code start from _start so lower address space can be stack
	//ldr		x1, =_start
	ldr		x1, =PA(_start)			// should use this version when switch the kernel mapping in linker script
	mov		sp, x1

	// setup the identity mapping for lab and enable MMU
	bl 		setup_identity_mapping

	// after enabling the MMU 
	// stack pointer need to be in the virtual address
	mov 	x1, #0
    movk 	x1, #0xffff, lsl 48

	mov		x0, sp
	orr		x0, x0, x1
	mov		sp, x0

    mov 	x0, fp
    orr 	fp, x0, x1

	ldr		x2, =kernel_main
	br		x2

kernel_main:
	bl		main					// branch label: will link in linking state
	b		halt

from_el2_to_el1:
	// mmu
	//ldr		x0, =SCTLR_VALUE_MMU_DISABLED
	//msr		sctlr_el1, x0
	// end mmu

	mov		x0, HCR_VALUE			    // RW, register width control
	msr		hcr_el2, x0
	mov		x0, SPSR_VALUE				// EL1h (SPSel = 1) with interrupt disabled
	msr		spsr_el2, x0
	msr		elr_el2, lr
	eret

halt:
	wfe
	b		halt

.section ".data"

// variables
.global _dtb_ptr

_dtb_ptr: .quad 0x0

