#include "../include/mm.h"

.section ".text.boot"

.globl _start
_start:
	//  Stroe the value of x0
	mov     x10, x0
    mov     x11, x1
    mov     x12, x2
    mov     x13, x3
	
	mrs	x0, mpidr_el1		
	and	x0, x0,#0xFF		// Check processor id
	cbz	x0, master		// Hang for all non-primary CPU
	b	proc_hang

proc_hang: 
	b 	proc_hang

master:
	adr	x0, bss_begin
	adr	x1, bss_end
	sub	x1, x1, x0
	bl 	memzero

	bl from_el2_to_el1  // change el2 -> el1

    // set exception vector table
    adr x0, exception_vector_table
    msr vbar_el1, x0

	// move value of x10~x13 to x0~x3	
	mov     x0, x10
    mov     x1, x11
    mov     x2, x12
    mov     x3, x13
	
	mov	sp, #LOW_MEMORY
	bl	kernel_main
	b 	proc_hang		// should never come here

from_el2_to_el1:
    mov x0, (1 << 31) // EL1 uses aarch64
    msr hcr_el2, x0
    mov x0, 0x3c5 // EL1h (SPSel = 1) with interrupt disabled
    msr spsr_el2, x0
    msr elr_el2, lr
    eret // return to EL1

// save general registers to stack
.macro save_all
    sub sp, sp, 36 * 8
    stp x0, x1, [sp ,16 * 0]
    stp x2, x3, [sp ,16 * 1]
    stp x4, x5, [sp ,16 * 2]
    stp x6, x7, [sp ,16 * 3]
    stp x8, x9, [sp ,16 * 4]
    stp x10, x11, [sp ,16 * 5]
    stp x12, x13, [sp ,16 * 6]
    stp x14, x15, [sp ,16 * 7]
    stp x16, x17, [sp ,16 * 8]
    stp x18, x19, [sp ,16 * 9]
    stp x20, x21, [sp ,16 * 10]
    stp x22, x23, [sp ,16 * 11]
    stp x24, x25, [sp ,16 * 12]
    stp x26, x27, [sp ,16 * 13]
    stp x28, x29, [sp ,16 * 14]
    str x30, [sp, 16 * 15]

    // push information for nested interrupt
    mrs     x0,       spsr_el1
    str     x0,       [sp, 16 * 16] // push spsr_el1
    mrs     x0,       elr_el1
    str     x0,       [sp, 16 * 17] // push elr_el1

.endm

// load general registers from stack
.macro load_all
    ldp x0, x1, [sp ,16 * 0]
    ldp x2, x3, [sp ,16 * 1]
    ldp x4, x5, [sp ,16 * 2]
    ldp x6, x7, [sp ,16 * 3]
    ldp x8, x9, [sp ,16 * 4]
    ldp x10, x11, [sp ,16 * 5]
    ldp x12, x13, [sp ,16 * 6]
    ldp x14, x15, [sp ,16 * 7]
    ldp x16, x17, [sp ,16 * 8]
    ldp x18, x19, [sp ,16 * 9]
    ldp x20, x21, [sp ,16 * 10]
    ldp x22, x23, [sp ,16 * 11]
    ldp x24, x25, [sp ,16 * 12]
    ldp x26, x27, [sp ,16 * 13]
    ldp x28, x29, [sp ,16 * 14]
    ldr x30, [sp, 16 * 15]

    // pop information for nested interrupt
    ldr     x0,       [sp, 16 * 16] 
    msr     spsr_el1, x0            // pop spsr_el1
    ldr     x0,       [sp, 16 * 17]
    msr     elr_el1,  x0            // pop elr_el1

    add sp, sp, 36 * 8
.endm

.macro exception_entry label
    b      \label
    .align 7    
.endm

.align 11 // vector table should be aligned to 0x800
.global exception_vector_table
exception_vector_table:
    // Exception from the current EL while using SP_EL0
    exception_entry exception_invalid
    exception_entry exception_invalid      
    exception_entry exception_invalid
    exception_entry exception_invalid

    // Exception from the current EL while using SP_ELx
    exception_entry el1_sync
    exception_entry el1_irq                
    exception_entry exception_invalid
    exception_entry exception_invalid

    // Exception from a lower EL and at least one lower EL is AArch64
    exception_entry el0_sync
    exception_entry el0_irq
    exception_entry exception_invalid
    exception_entry exception_invalid

    // Exception from a lower EL and all lower EL are AArch32
    exception_entry exception_invalid
    exception_entry exception_invalid
    exception_entry exception_invalid
    exception_entry exception_invalid

exception_invalid:
    save_all
    bl exception_invalid_handler
    load_all
    eret

el0_sync:
    save_all
    bl except_handler_c
    load_all
    eret

el1_sync:
    save_all
    bl except_handler_c
    load_all
    eret

el0_irq:
    save_all
    bl exception_el1_irq_handler
    load_all
    eret

el1_irq:
    save_all
    bl exception_el1_irq_handler
    load_all
    eret