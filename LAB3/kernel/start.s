.section ".text.kernel"
.globl _start

_start:
    ldr    x1, =_dtb_ptr
    str    x0, [x1]

    /*          cpu id pass         */
    mrs    x20, mpidr_el1        
    and    x20, x20,#0xFF        // Check processor id
    cbz    x20, master        // Hang for all non-primary CPU

hang:
    b hang

master:
    bl     from_el2_to_el1   
      
    // Set Up interrupt vector table
    ldr x0, =el1_vector_base
    msr vbar_el1, x0

    adr    x20, _sbss
    adr    x21, _ebss
    sub    x21, x21, x20
    bl     memzero

    mov    sp, #0x200000  // 2MB
    bl     kernel_main

from_el2_to_el1:
	mov x0, (1 << 31) // EL1 uses aarch64
	msr hcr_el2, x0
	mov x0, 0x3c5 // EL1h (SPSel = 1) with interrupt disabled
	msr spsr_el2, x0
	msr elr_el2, lr
	eret // return to EL1
    

.global _dtb_ptr
.section .data
_dtb_ptr: .dc.a 0x0
