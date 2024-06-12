.section ".text.boot"

.globl _start
_start:
	mrs	x0, mpidr_el1		
	and	x0, x0,#0xFF	// Check processor id
	cbz	x0, master		// if cpu_id == 0, then booting
	b	proc_hang       // Otherwise, hang

proc_hang: 
	b 	proc_hang

master:
	bl from_el2_to_el1

exception_table:
 	bl set_exception_vector_table

bss_init:
	adr	x0, __bss_start
	adr	x1, __bss_size
	bl 	memzero

	adr x1, _start
	mov sp, x1
	bl	main     // jump to kernel_main c code
	b 	proc_hang		// should never come here

memzero:
    str xzr, [x0], #8   // xzr: a register holds zero
	subs x1, x1, #8
	b.gt memzero
	ret

// adr: Load a label's relative address into the target register.

set_exception_vector_table:
    adr x0, exception_vector_table  // get the base address of the exception vector table
    msr vbar_el1, x0                // set the base address of the exception vector table
    ret

from_el2_to_el1:
    mov x0, (1 << 31)   // EL1 uses aarch64
    msr hcr_el2, x0     // msr: Status Register <-- Register (page 1924)
    mov x0, 0x3c5       // EL1h (SPSel = 1) with interrupt disabled (D = 1, A = 1, I = 1, F = 1, M[3:0] = 5)
    msr spsr_el2, x0    // Save the current processor's state
    msr elr_el2, lr     // Save the exception return address
    eret                // return to EL1 (use eret to return from the exception)